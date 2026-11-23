// SPDX-License-Identifier: GPL-3.0-only
// test_world.cpp — headless World + embedded sclang → audio.
//
// Goal: confirm that a headless World (libscsynth) can:
//   1. Receive OSC from an embedded sclang Server (same process, over local UDP).
//   2. Execute the commands (units > 0).
//   3. Render the graph with World_Run (canonical NRT loop, SC_World.cpp:563).
//   4. Produce audible audio on mAudioBus (→ WAV).
//
// Acceptance: units > 0, non-silent WAV and a fundamental tone ≈ 440 Hz.
//
// ROOT CAUSE of the previous failure (units=0, silent audio): in NRT
// (mRealTime=false) there is no audio driver, and the normal path World_OpenUDP
// → ProcessOSCPacket (server/scsynth/SC_CoreAudio.cpp:179) DROPS the datagram
// when there is no driver (lines 186-189). The World never executed a single
// command.
// FIX (replicating World_NonRealTimeSynthesis, SC_World.cpp:563):
//   - OWN UDP socket on 127.0.0.1:port (not World_OpenUDP).
//   - On every block of the render loop: drain the socket (non-blocking
//     recvfrom) and dispatch with PerformOSCMessage/PerformOSCBundle
//     (SC_CoreAudio.cpp:192+; they do NOT need a driver), with our own
//     ReplyAddress that answers the sender through sendto.
//   - World_Run(world) renders mBufLength frames to mAudioBus.
// This is exactly the external sc3~ pattern: audio thread = drain + Perform* +
// World_Run + read mAudioBus; main thread = sclang.
//
// sclang 3.14.1 (SCClassLibrary/Common/Control/Server.sc):
//   - Server.default == Server.new(\localhost, NetAddr("127.0.0.1", 57110))
//     (line 385) → it points at our port; no other Server has to be created.
//   - There is no s.waitFor in 3.14.1 → use s.waitForBoot { |onComplete| }.
//   - Server.boot pings /sync (prPingApp, ~line 1278); if the server answers it
//     does NOT fork scsynth → the engine loop has to be draining WHILE booting.
//
// NOTE: bare binary → it needs SCClassLibrary in its dir (symlink).
// Default port 57110 (collides with the IDE → pass it through argv if needed).
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "OSC_Packet.h" // server/scsynth: struct OSC_Packet (linkage nativa)
#include "SC_World.h"
#include "SC_WorldOptions.h"
#include "TestClient.h"

// World API not exposed in public headers. NOTE: C++ linkage (mangled): they
// are OUTSIDE extern "C" in SC_Prototypes.h / SC_CoreAudio.cpp. They are
// declared with C++ linkage.
void World_Run(struct World* inWorld);
void World_Start(struct World* inWorld);
void World_SetSampleRate(struct World* inWorld, double inSampleRate);
int PerformOSCMessage(struct World* inWorld, int inSize, char* inData, ReplyAddress* inAddr);
void PerformOSCBundle(struct World* inWorld, struct OSC_Packet* inPacket);

static bool gAllOk = true;
static bool check(bool cond, const char* what) {
    gAllOk = gAllOk && cond;
    std::printf("[%s] %s\n", cond ? "OK " : "FAIL", what);
    return cond;
}
static bool check(bool cond, const std::string& what) {
    gAllOk = gAllOk && cond;
    std::printf("[%s] %s\n", cond ? "OK " : "FAIL", what.c_str());
    return cond;
}

// --- reply address: answers the datagram sender -------------------------------
// Real ReplyAddress (common/SC_ReplyImpl.hpp):
// mAddress/mPort/mReplyData/mReplyFunc. Perform* uses it to sendto() the reply
// to the source, which is captured with recvfrom in the drain (gReplyPeer;
// synchronous loop → global).
static int gReplySock = -1;
static struct sockaddr_in gReplyPeer{};
static bool gDebugOsc = false;

static void udpReplyFunc(struct ReplyAddress* addr, char* data, int size) {
    (void)addr;
    if (gReplySock < 0)
        return;
    if (gDebugOsc)
        std::fprintf(stderr, "[osc reply] size=%d\n", size);
    sendto(gReplySock, data, size, 0,
           reinterpret_cast<struct sockaddr*>(&gReplyPeer), sizeof(gReplyPeer));
}

// --- socket UDP propio (reemplaza a World_OpenUDP) ----------------------------
static int bindUdp(int port) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        return -1;
    int one = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(port);
    if (bind(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) != 0) {
        close(sock);
        return -1;
    }
    fcntl(sock, F_SETFL, O_NONBLOCK);
    return sock;
}

// --- WAV 16-bit PCM -------------------------------------------------------------
static bool writeWav16(const char* path, int sampleRate,
                       const std::vector<std::vector<int16_t>>& channels) {
    size_t numFrames = channels.empty() ? 0 : channels[0].size();
    int numCh = (int)channels.size();
    uint32_t dataSize = (uint32_t)(numFrames * numCh * 2);
    uint32_t byteRate = (uint32_t)sampleRate * numCh * 2;
    uint16_t blockAlign = (uint16_t)(numCh * 2);

    // Layout RIFF/WAVE canonical (offsets exactos, little-endian):
    //  0 RIFF | 4 chunkSize | 8 WAVE | 12 "fmt " | 16 subchunk1Size(16)
    // 20 audioFormat(1=PCM) | 22 numChannels | 24 sampleRate | 28 byteRate
    // 32 blockAlign | 34 bitsPerSample | 36 "data" | 40 dataSize
    uint8_t hdr[44] = {0};
    memcpy(hdr + 0, "RIFF", 4);
    auto put32 = [&hdr](int off, uint32_t v) {
        hdr[off] = v & 0xff;
        hdr[off + 1] = (v >> 8) & 0xff;
        hdr[off + 2] = (v >> 16) & 0xff;
        hdr[off + 3] = (v >> 24) & 0xff;
    };
    auto put16 = [&hdr](int off, uint16_t v) {
        hdr[off] = v & 0xff;
        hdr[off + 1] = (v >> 8) & 0xff;
    };
    put32(4, 36 + dataSize);
    memcpy(hdr + 8, "WAVE", 4);
    memcpy(hdr + 12, "fmt ", 4);
    put32(16, 16);
    put16(20, 1); // PCM
    put16(22, (uint16_t)numCh);
    put32(24, (uint32_t)sampleRate);
    put32(28, byteRate);
    put16(32, blockAlign);
    put16(34, 16);
    memcpy(hdr + 36, "data", 4);
    put32(40, dataSize);

    FILE* f = fopen(path, "wb");
    if (!f)
        return false;
    fwrite(hdr, 1, 44, f);
    for (size_t i = 0; i < numFrames; ++i)
        for (int c = 0; c < numCh; ++c)
            fwrite(&channels[c][i], 2, 1, f);
    fclose(f);
    return true;
}

int main(int argc, char** argv) {
    gDebugOsc = std::getenv("SC3_DEBUG_OSC") != nullptr;
    const int port = (argc > 1) ? std::atoi(argv[1]) : 57110;
    // The UGens are NOT compiled into libscsynth (the build does not use
    // STATIC_PLUGINS): they are loaded from .scx at runtime. If
    // mUGensPluginPath is passed it is EXCLUSIVE (SC_Lib_Cintf.cpp:171: "If
    // uGensPluginPath is supplied, it is exclusive") => everything we want has
    // to be listed. Path separator: SC_STRPARSE_PATHDELIMITER (\':\'). This is
    // exactly PLAN decision 3: packaged stock + user plugins dir.
    const char* stockUGens = "../vendor/supercollider/build/server/plugins";
    const char* home = std::getenv("HOME");
    std::string userPlugins = home
        ? std::string(home) + "/Library/Application Support/SuperCollider/Extensions"
        : "";
    std::string ugensPath = (argc > 2)
        ? std::string(argv[2])
        : std::string(stockUGens) + ":" + userPlugins;
    const int durationMs = 1500;

    // --- 1. Own UDP socket -------------------------------------------------------
    // It is bound BEFORE booting sclang: if its startup file ends up sending OSC
    // (e.g. s.quit), the datagrams stay queued in the socket instead of being lost
    // to ICMP port-unreachable (the World does not exist yet).
    int sock = bindUdp(port);
    if (sock < 0) {
        std::printf("[FAIL] bind UDP 127.0.0.1:%d (IDE open?)\n", port);
        return 1;
    }
    gReplySock = sock;

    // --- 2. embedded sclang FIRST (order "sclang → s.options → World") -----------
    // Reason: the user s.options live in their startup.scd, which is evaluated
    // during compileLibrary. Creating the World before ignored them (measured:
    // maxNodes 1024 instead of 32768, memSize 8192 instead of 262144). SCMaxCompat
    // neutralizes s.boot/s.quit (otherwise sclang would fork a real scsynth and
    // fight our World).
    TestClient client;
    if (!client.boot(false)) {
        std::printf("[FAIL] sclang boot (stock tampoco compila)\n");
        return 1;
    }
    if (client.mDegraded) {
        // In the external this goes to the `error` outlet, not
        // to stdout.
        std::printf("[WARN] class library del usuario degradada:\n%s", client.mDegradeReport.c_str());
        if (!client.mErr.empty())
            std::printf("  errores del startup tras degradar:\n%s", client.mErr.c_str());
    }

    // --- 3. s.options del usuario → WorldOptions ---------------------------------
    TestClient::ServerOpts so = client.readServerOptions();
    const int sampleRate = so.sampleRateSet ? (int)std::lround(so.sampleRate) : 48000;
    const int bufLength = (int)so.blockSize;
    std::printf("[info] s.options: maxNodes=%ld memSize=%ldKB numWireBufs=%ld numRGens=%ld\n"
                "       numBuffers=%ld numAudioBusChannels=%ld (in=%ld out=%ld)\n"
                "       blockSize=%ld maxLogins=%ld sampleRate=%d%s\n",
                so.maxNodes, so.memSize, so.numWireBufs, so.numRGens, so.numBuffers,
                so.numAudioBusChannels, so.numInputBusChannels, so.numOutputBusChannels,
                so.blockSize, so.maxLogins, sampleRate, so.sampleRateSet ? "" : " (default)");

    // --- 4. headless World (NRT) with the user options
    // ----------------------------
    WorldOptions opt;
    opt.mRealTime = false;
    opt.mPreferredSampleRate = sampleRate;
    opt.mBufLength = bufLength;
    // MANDATORY consistency: mAudioBus is allocated with mBufLength *
    // mNumAudioBusChannels, but the layout the UGens use is
    // [out0][out1]...[in0][in1]... with *BusChannels as width. If the user has
    // fewer buses than in+out, the high buses fall OUTSIDE the allocation (heap
    // overflow) => we widen it to the consistent minimum.
    const long buses = std::max<long>(so.numAudioBusChannels,
                                      so.numInputBusChannels + so.numOutputBusChannels);
    opt.mNumAudioBusChannels = (uint32)buses;
    opt.mNumInputBusChannels = (uint32)so.numInputBusChannels;
    opt.mNumOutputBusChannels = (uint32)so.numOutputBusChannels;
    opt.mNumBuffers = (uint32)std::max<long>(so.numBuffers, 8);
    opt.mMaxNodes = (uint32)std::max<long>(so.maxNodes, 1024);
    opt.mMaxGraphDefs = (uint32)std::max<long>(so.maxSynthDefs, 1024);
    opt.mRealTimeMemorySize = (uint32)std::max<long>(so.memSize, 8192);
    opt.mMaxWireBufs = (uint32)std::max<long>(so.numWireBufs, 64);
    opt.mNumRGens = (uint32)std::max<long>(so.numRGens, 64);
    // It has to be <= 32: sclang validates ServerOptions:maxLogins (<=32)
    // when receiving /done from /notify; with 64 (the WorldOptions default)
    // it raises Error and does NOT set notified => serverRunning stays false.
    opt.mMaxLogins = (uint32)std::min<long>(std::max<long>(so.maxLogins, 1), 32);
    opt.mVerbosity = 1;
    opt.mUGensPluginPath = ugensPath.c_str();
    std::printf("[info] mUGensPluginPath = %s\n", ugensPath.c_str());
    World* world = World_New(&opt);
    if (!world) {
        std::printf("[FAIL] World_New\n");
        return 1;
    }
    World_SetSampleRate(world, sampleRate);
    check(world->mBufLength == bufLength &&
              world->mNumAudioBusChannels == (uint32)buses &&
              world->mNumSndBufs == opt.mNumBuffers &&
              world->mNumRGens == opt.mNumRGens,
          std::string("World created with the user's s.options (bufLength ") +
              std::to_string(world->mBufLength) + ", buses " +
              std::to_string(world->mNumAudioBusChannels) + ", sndBufs " +
              std::to_string(world->mNumSndBufs) + ", rgens " +
              std::to_string(world->mNumRGens) + ")");

    World_Start(world);
    std::printf("[OK ] World NRT + UDP 127.0.0.1:%d (drain propio)\n", port);

    // --- 4. Engine loop: drain + Perform* + World_Run + tick ---------------------
    // A single loop in this test (main thread); in the external the drain goes on
    // the audio thread and the tick/evaluate on the main one.
    auto step = [&](std::vector<float>& out) {
        for (;;) {
            char buf[65536];
            struct sockaddr_in from{};
            socklen_t fromLen = sizeof(from);
            ssize_t n = recvfrom(sock, buf, sizeof(buf), 0,
                                 reinterpret_cast<struct sockaddr*>(&from), &fromLen);
            if (n <= 0)
                break;
            // Traza OSC opcional: SC3_DEBUG_OSC=1 ./test_world
            // (incluye dump crudo a /tmp/osc_recv.bin, formato [u32 LE size][data]).
            if (gDebugOsc) {
                static int gRecvCount = 0;
                std::fprintf(stderr, "[osc #%d from %d] n=%d : ", gRecvCount++,
                             (int)ntohs(from.sin_port), (int)n);
                for (int i = 0; i < n && i < 64; ++i)
                    std::fprintf(stderr, "%02x ", (unsigned char)buf[i]);
                std::fprintf(stderr, "\n");
                static std::FILE* f = std::fopen("/tmp/osc_recv.bin", "wb");
                if (f) {
                    uint32_t sz = (uint32_t)n;
                    std::fwrite(&sz, 4, 1, f);
                    std::fwrite(buf, 1, (size_t)n, f);
                    std::fflush(f);
                }
            }
            gReplyPeer = from; // remitente (para udpReplyFunc)
            ReplyAddress ra{};
            ra.mReplyData = &gReplySock;
            ra.mReplyFunc = udpReplyFunc;
            // bundle detection same as gIsBundle (SC_CoreAudio.cpp): "#bundle".
            // CAREFUL: numeric commands start with 0x00 (inData[0]==0 => index in
            // inData[3], see PerformOSCMessage) => compare the 8 bytes of the tag,
            // not just buf[0]/buf[1]. A bundle can be EMPTY: 16 bytes (#bundle\\0
            // + timetag, 0 msgs). sclang sends an empty one after /sync; with n >
            // 16 it fell into PerformOSCMessage => "FAILURE IN SERVER: #bundle
            // Command not found".
            if (n >= 16 && std::memcmp(buf, "#bundle\0", 8) == 0) {
                OSC_Packet pkt{};
                pkt.mData = buf;
                pkt.mSize = (int)n;
                pkt.mReplyAddr = ra;
                PerformOSCBundle(world, &pkt);
            } else {
                PerformOSCMessage(world, (int)n, buf, &ra);
            }
        }
        // KEY: mBufCounter has to be incremented after every block (same as ALL
        // drivers: SC_CoreAudio/SC_Jack/SC_PortAudio/SC_Bela do `for (i...;
        // world->mBufCounter++)`). The audio bus is NOT cleared with memset:
        // the clearing is implicit through mAudioBusTouched. Out_next_a
        // (server/plugins/IOUGens.cpp) does: if (touched[i] == bufCounter)
        // Accum(out, in); else { Copy(out, in); touched[i] = bufCounter; } If
        // mBufCounter does not advance, touched[i] stays == bufCounter and
        // EVERYTHING accumulates on the previous block → DC offset + corrupted
        // amplitude.
        World_Run(world);
        world->mBufCounter++;
        out.resize(bufLength * 2);
        for (int i = 0; i < bufLength; ++i) {
            out[i] = world->mAudioBus[i];
            out[bufLength + i] = world->mAudioBus[bufLength + i];
        }
        client.tick();
    };

    // Polling helper: iterates the engine until cond() or the
    // timeout.
    auto pumpUntil = [&](auto cond, int maxIters) -> bool {
        std::vector<float> out;
        for (int i = 0; i < maxIters; ++i) {
            step(out);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            if (cond())
                return true;
        }
        return cond();
    };

    // --- 5. Adoption of the already running server (s = Server.default = 57110)
    // ---- The World already exists (created with the user s.options) and sclang
    // must NOT boot it: s.boot/s.quit are no-ops (SCMaxCompat). sclang cannot
    // "adopt" an already running UDP server through s.boot → we replicate what
    // bootServerApp does for us: hasBooted + alive thread (poll /status) + /notify
    // handshake.
    client.evaluate("OSCFunc({ |msg| 'OSC-IN: %'.format(msg).postln; }, nil);");
    // CAREFUL: hasBooted is getter-only (var <hasBooted) => assigning it
    // raises DoesNotUnderstandError. The public setter is serverRunning_ ,
    // which does hasBooted = running (ServerStatus.sc:183). Also
    // serverRunning is the check SynthDef:send uses to decide whether it
    // sends the /d_recv.
    client.clearPost();
    client.evaluate(
        "s.statusWatcher.serverRunning = true;\n"
        "s.statusWatcher.prSendNotifyRequest(true, true);");
    if (gDebugOsc)
        std::fprintf(stderr, "[post] %s[err] %s\n", client.mPost.c_str(), client.mErr.c_str());
    // Poll of the LIVE value: it is re-evaluated inside the loop. (Before, it
    // read the post of a single previous evaluation => the check passed by
    // coincidence, due to text from another log, not the value.)
    bool running = false;
    std::vector<float> outN;
    for (int i = 0; i < 400 && !running; ++i) {
        step(outN);
        client.clearPost();
        client.evaluate("s.serverRunning.postln;");
        running = client.mPost.find("true") != std::string::npos;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    check(running, "server running (s.serverRunning)");
    std::printf("  post: %s\n  err: %s\n", client.mPost.c_str(), client.mErr.c_str());

    // --- 6. SynthDef + Synth -----------------------------------------------------
    client.clearPost();
    client.evaluate(
        "SynthDef(\\sine440, { Out.ar(0, SinOsc.ar(440, 0, 0.8) ) }).add;");
    if (gDebugOsc)
        std::fprintf(stderr, "[post] %s[err] %s\n", client.mPost.c_str(), client.mErr.c_str());
    // Wait for the /c_newArrival datagram... (/c_ SynthDef) to arrive and be
    // processed: evaluating in sclang is async → several steps with
    // draining.
    for (int i = 0; i < 25; ++i) {
        std::vector<float> out;
        step(out);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    // Measure the DELTA: the user startup.scd and their Quarks may have
    // created nodes/UGens of their own, so "units > 0" says nothing about our
    // synth. What matters is that OUR /s_new creates units.
    const int unitsBefore = world->mNumUnits;
    client.evaluate("x = Synth(\\sine440);");
    bool unitsOk = pumpUntil([&] { return world->mNumUnits > unitsBefore; }, 500);
    check(unitsOk, std::string("units > 0 (antes ") + std::to_string(unitsBefore) +
                       ", ahora " + std::to_string(world->mNumUnits) + ")");

    // --- 7. Captura de audio -----------------------------------------------------
    const long totalFrames = sampleRate * durationMs / 1000;
    const int blocks = totalFrames / bufLength;
    std::vector<std::vector<float>> raw(2);
    raw[0].reserve(totalFrames);
    raw[1].reserve(totalFrames);
    for (int b = 0; b < blocks; ++b) {
        std::vector<float> out;
        step(out);
        for (int i = 0; i < bufLength; ++i) {
            raw[0].push_back(out[i]);
            raw[1].push_back(out[bufLength + i]);
        }
    }

    double rms = 0.0;
    for (float v : raw[0])
        rms += v * v;
    rms = std::sqrt(rms / raw[0].size());
    check(rms > 0.01, std::string("RMS > 0.01 (medido ") + std::to_string(rms) + ")");

    // Waveform: a clean SinOsc.ar(440,0,0.8) should give DC ~0 and amplitude
    // ~0.8 (RMS = 0.8/sqrt(2) = 0.5657). If the audio bus accumulates between
    // blocks (mBufCounter not advancing) weird DC and amplitude show up
    // (observed symptom: DC 0.22, min -0.53, max 0.83) => regression check.
    double dc = 0.0;
    double peak = 0.0;
    for (float v : raw[0]) {
        dc += v;
        peak = std::max(peak, (double)std::fabs(v));
    }
    dc /= raw[0].size();
    check(std::fabs(dc) < 0.01 && peak > 0.7 && peak < 0.9,
          std::string("forma de onda (DC ") + std::to_string(dc) + ", peak " +
              std::to_string(peak) + " ~ 0.8)");

    // --- 8. Tone verification (zero crossings) ----------------------------------
    int crossings = 0;
    for (size_t i = 1; i < raw[0].size(); ++i)
        if ((raw[0][i - 1] < 0 && raw[0][i] >= 0))
            crossings++;
    // We count ONLY ascending crossings (prev<0 && cur>=0) => one per cycle
    // => Hz = crossings / seconds (the previous /2.0 counted twice).
    double estFreq = crossings / (durationMs / 1000.0);
    check(std::abs(estFreq - 440.0) < 15.0,
          std::string("tono ≈ 440 Hz (estimado ") + std::to_string((int)estFreq) + " Hz)");

    // --- 9. WAV -------------------------------------------------------------------
    std::vector<std::vector<int16_t>> pcm(2);
    for (int c = 0; c < 2; ++c) {
        pcm[c].resize(raw[c].size());
        for (size_t i = 0; i < raw[c].size(); ++i)
            pcm[c][i] = (int16_t)(std::max(-1.0f, std::min(1.0f, raw[c][i])) * 32767.0);
    }
    bool wrote = writeWav16("test_world_out.wav", sampleRate, pcm);
    check(wrote, "WAV escrito (test_world_out.wav)");

    // --- 10. Cleanup ---------------------------------------------------------------
    client.evaluate("x.free; s.quit;");
    pumpUntil([] { return true; }, 1);
    World_Cleanup(world, true);
    client.shutdownLibrary();
    client.shutdownRuntime();
    std::printf("== test_world: DONE ==\n");
    return (gAllOk && unitsOk && wrote) ? 0 : 1;
}
