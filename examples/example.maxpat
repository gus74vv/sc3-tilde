{
    "patcher": {
        "fileversion": 1,
        "appversion": {
            "major": 9,
            "minor": 2,
            "revision": 0,
            "architecture": "x64",
            "modernui": 1
        },
        "classnamespace": "box",
        "rect": [ 43.0, 126.0, 1199.0, 710.0 ],
        "boxes": [
            {
                "box": {
                    "id": "obj-27",
                    "linecount": 13,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 990.0, 528.0, 150.0, 181.0 ],
                    "text": "<- sc_code colors: `@theme onedark` is the default (menu in the inspector). Also catppuccin,\n dracula, nord, gruvbox, monokai, solarized.\n   Or send the message `theme dracula`; `themes` posts the list.\n   A color set by hand (`@bgcolor …`) wins over the theme."
                }
            },
            {
                "box": {
                    "id": "obj-3",
                    "lastchannelcount": 0,
                    "maxclass": "live.gain~",
                    "numinlets": 2,
                    "numoutlets": 5,
                    "outlettype": [ "signal", "signal", "", "float", "list" ],
                    "parameter_enable": 1,
                    "patching_rect": [ 360.0, 878.0, 48.0, 136.0 ],
                    "saved_attribute_attributes": {
                        "valueof": {
                            "parameter_longname": "live.gain~[1]",
                            "parameter_mmax": 6.0,
                            "parameter_mmin": -70.0,
                            "parameter_modmode": 3,
                            "parameter_shortname": "live.gain~[1]",
                            "parameter_type": 0,
                            "parameter_unitstyle": 4
                        }
                    },
                    "varname": "live.gain~[1]"
                }
            },
            {
                "box": {
                    "id": "obj-25",
                    "linecount": 5,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 1008.0, 90.0, 150.0, 74.0 ],
                    "text": "<--press just once and forget to generate the index file for autocompletion. ---> Instructions:"
                }
            },
            {
                "box": {
                    "id": "obj-5",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 962.0, 90.0, 37.0, 22.0 ],
                    "text": "index"
                }
            },
            {
                "box": {
                    "id": "obj-46",
                    "linecount": 13,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 902.0, 166.0, 500.0, 181.0 ],
                    "text": "Patch sc_code's eval outlet to sc3~ and send `index` once. sclang writes\n~/Documents/Max 9/Library/sc_code-index.txt (~32.300 entries, ONE file shared by\nevery sc_code) and each sc_code reads it by itself; `index <path>` is a one shot\nand @indexfile <path> overrides it. Send `index` again after installing Quarks,\ntouching Extensions or updating SC.\nCALL AID: @callaid 1 (default) reserves the bottom line: with the caret inside\nSinOsc.ar( it shows the signature, highlights the current argument and shows n/N.\nCOMPLETION (display only, nothing is written while typing): after 3 letters the bar\nshows the best candidate. Tab inserts it (one undo step), Shift+Tab cycles, Esc\ncloses, typing filters. Return is never taken (it still evaluates / breaks the line).\nAfter `Class.` the list is only that class's methods. Ctrl+Space opens the list\nwith no minimum prefix (macOS may eat the key). Tab at a clean argument start writes\nthe IDE template: SinOsc.ar( + Tab -> `freq: `."
                }
            },
            {
                "box": {
                    "id": "obj-7",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 668.0, 892.0, 150.0, 20.0 ],
                    "text": "connect to sc3~ inlet"
                }
            },
            {
                "box": {
                    "id": "obj-45",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 362.0, 129.0, 288.0, 20.0 ],
                    "text": "2 - Turn on Max's DSP and evaluate some sc code"
                }
            },
            {
                "box": {
                    "id": "obj-41",
                    "linecount": 4,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 766.0, 71.0, 150.0, 60.0 ],
                    "text": "pass this path to sc_code inlet or put the path in the inspector. The path is saved with the patch."
                }
            },
            {
                "box": {
                    "id": "obj-39",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 739.0, 43.0, 190.0, 22.0 ],
                    "text": "snippets ~/path/to/my_snippets.txt"
                }
            },
            {
                "box": {
                    "id": "obj-37",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 667.0, 913.0, 151.0, 22.0 ],
                    "text": "set x freq 220 amp 0.2"
                }
            },
            {
                "box": {
                    "id": "obj-35",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 699.0, 1096.0, 32.0, 22.0 ],
                    "text": "print"
                }
            },
            {
                "box": {
                    "id": "obj-34",
                    "maxclass": "number",
                    "numinlets": 1,
                    "numoutlets": 2,
                    "outlettype": [ "", "bang" ],
                    "parameter_enable": 0,
                    "patching_rect": [ 699.0, 969.0, 50.0, 22.0 ]
                }
            },
            {
                "box": {
                    "id": "obj-22",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 699.0, 1066.0, 78.0, 22.0 ],
                    "text": "prepend eval"
                }
            },
            {
                "box": {
                    "id": "obj-29",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 699.0, 1021.0, 174.0, 22.0 ],
                    "text": "sprintf symout x.set(\\\\freq\\, %ld)"
                }
            },
            {
                "box": {
                    "id": "obj-20",
                    "lastchannelcount": 0,
                    "maxclass": "live.gain~",
                    "numinlets": 2,
                    "numoutlets": 5,
                    "outlettype": [ "signal", "signal", "", "float", "list" ],
                    "parameter_enable": 1,
                    "patching_rect": [ 143.0, 567.0, 49.0, 142.0 ],
                    "saved_attribute_attributes": {
                        "valueof": {
                            "parameter_longname": "live.gain~",
                            "parameter_mmax": 6.0,
                            "parameter_mmin": -70.0,
                            "parameter_modmode": 3,
                            "parameter_shortname": "live.gain~",
                            "parameter_type": 0,
                            "parameter_unitstyle": 4
                        }
                    },
                    "varname": "live.gain~"
                }
            },
            {
                "box": {
                    "id": "obj-13",
                    "linecount": 2,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 204.0, 532.0, 150.0, 33.0 ],
                    "text": "<- equals [sc3~ 2 0] 2ch, bus 0 & 1"
                }
            },
            {
                "box": {
                    "id": "obj-9",
                    "linecount": 8,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 432.0, 839.0, 150.0, 114.0 ],
                    "text": "<-second instance > 2ch, bus 2 & 3. Independent output. Don't need to initialize. It will attach on eval. \naudio input (only with @ins N). SoundIn.ar(i) reads inlet i"
                }
            },
            {
                "box": {
                    "id": "obj-2",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 4,
                    "outlettype": [ "", "", "signal", "signal" ],
                    "patching_rect": [ 303.0, 839.0, 105.0, 22.0 ],
                    "text": "sc3~ 2 2"
                }
            },
            {
                "box": {
                    "blinkduration": 600,
                    "id": "obj-33",
                    "indexfile": "",
                    "maxclass": "sc_code",
                    "numinlets": 1,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 474.0, 498.0, 508.0, 295.5 ],
                    "sc_code_caret": 233,
                    "sc_code_scroll": 0,
                    "sc_code_text": "// sc3~ `set` example (see tests/args.maxpat): the [number] boxes drive\n//   set x freq <n>   →  x.set(\\freq, <n>);\n//   set x amp  <n>   →  x.set(\\amp,  <n>);\n(\nx = { SinOsc.ar(\\freq.kr(242)) * \\amp.kr(0.2) ! 2 }.play;\n)\nx.free;",
                    "snippets": ""
                }
            },
            {
                "box": {
                    "id": "obj-15",
                    "linecount": 9,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 837.0, 834.5, 479.0, 127.0 ],
                    "text": "sc3~ `set`: Synth arguments from Max UI, no escaping.\n[number] -> [prepend set x freq] -> sc3~   generates  x.set(\\freq, <n>);\nDrag/type in the number boxes; the last message box sends two pairs at once.\nEval x in sc_code first (it plays x = { |freq = 242 amp = 0.2| ... }).\n\nDo NOT write `eval x.set(\\freq, $1)` in a message box: Max splits it at the\ncomma and eats the backslash (it arrives as `x.set(freq\\, 81)`). If you need\nraw code with commas/symbols, use [sprintf symout x.set(\\\\\\freq, %ld)] \n(92 = backslash, 44 = comma)."
                }
            },
            {
                "box": {
                    "id": "obj-30",
                    "maxclass": "number",
                    "numinlets": 1,
                    "numoutlets": 2,
                    "outlettype": [ "", "bang" ],
                    "parameter_enable": 1,
                    "patching_rect": [ 668.0, 839.0, 50.0, 22.0 ],
                    "saved_attribute_attributes": {
                        "valueof": {
                            "parameter_initial": [ 204 ],
                            "parameter_initial_enable": 1,
                            "parameter_invisible": 1,
                            "parameter_longname": "number[1]",
                            "parameter_modmode": 0,
                            "parameter_shortname": "number[1]",
                            "parameter_type": 3
                        }
                    },
                    "varname": "number[1]"
                }
            },
            {
                "box": {
                    "format": 6,
                    "id": "obj-28",
                    "maxclass": "flonum",
                    "maximum": 1.0,
                    "minimum": 0.0,
                    "numinlets": 1,
                    "numoutlets": 2,
                    "outlettype": [ "", "bang" ],
                    "parameter_enable": 1,
                    "patching_rect": [ 755.0, 839.0, 50.0, 22.0 ],
                    "saved_attribute_attributes": {
                        "valueof": {
                            "parameter_initial": [ 0.5 ],
                            "parameter_initial_enable": 1,
                            "parameter_longname": "number",
                            "parameter_mmax": 1.0,
                            "parameter_modmode": 3,
                            "parameter_shortname": "number",
                            "parameter_type": 0
                        }
                    },
                    "varname": "number"
                }
            },
            {
                "box": {
                    "id": "obj-26",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 755.0, 868.0, 77.0, 22.0 ],
                    "text": "set x amp $1"
                }
            },
            {
                "box": {
                    "id": "obj-23",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 668.0, 868.0, 75.0, 22.0 ],
                    "text": "set x freq $1"
                }
            },
            {
                "box": {
                    "id": "obj-21",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 269.0, 449.0, 54.0, 22.0 ],
                    "text": "eval 1+1"
                }
            },
            {
                "box": {
                    "id": "obj-19",
                    "maxclass": "ezdac~",
                    "numinlets": 2,
                    "numoutlets": 0,
                    "patching_rect": [ 138.0, 821.5, 45.0, 45.0 ]
                }
            },
            {
                "box": {
                    "blinkduration": 600,
                    "id": "obj-17",
                    "indexfile": "",
                    "maxclass": "sc_code",
                    "numinlets": 1,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 362.0, 166.0, 530.0, 291.0 ],
                    "sc_code_caret": 1332,
                    "sc_code_scroll": 0,
                    "sc_code_text": "/////////////////////// Evaluate a Region with Cmd + Return \n// - A region is surrounded by parentheses; to be interpreted as a region, the parenthesis must each be the only character on a line\n/////////////////////// Evaluate a line with Shift + Return\n// Region -->\n(\nNdef(\\a, {\n\tvar f=LocalIn.ar(2).tanh;\n\tvar k=Latch.kr(\n\t\tf[0].abs,\n\t\tImpulse.kr(1/4));\n\tLocalOut.ar(\n\t\tf+CombC.ar( \n\t\t\tBlip.ar(\n\t\t\t\t[4,6],\n\t\t\t\t100*k+50,\n\t\t\t\t0.9),\n\t\t\t1,k*0.3,50*f));\n\tf\n} * -20.dbamp\n).fadeTime_(6).play\n)\n\n// line  -->\n\nNdef(\\a).clear(6);\n\n///////////////////////////////////////\n\n(\nNdef(\\b, {\n\tvar k, y;\n\tLocalOut.ar(\n\t\tk=tanh(\n\t\t\t500*(\n\t\t\t\tPinkNoise.ar(1!2)**\n\t\t\t\t(8/(LagUD.ar(\n\t\t\t\t\ty=LocalIn.ar(2).range(0,0.9)+0.1,\n\t\t\t\t\t0,\n\t\t\t\t\t0.2\n\t\t\t\t)))\n\t\t\t)\n\t\t)\n\t);\n\tSplay.ar(k,1-y).sum;\n} * -20.dbamp\n).fadeTime_(6).play\n)\n\nNdef(\\b).clear(6);\n\n/////////////////////////////////////\n\n(\nNdef(\\c, {\n\tGVerb.ar(\n\t\tMix(\n\t\t\t9.collect{\n\t\t\t\tSetResetFF.ar(\n\t\t\t\t\tDust.ar(8.rand+1),\n\t\t\t\t\tDust.ar(8.rand+1)).lag*\n\t\t\t\tSinOsc.ar(\n\t\t\t\t\tLFNoise2.kr(0.1,25,exprand(40,1500)\n\t\t\t\t\t)\n\t\t\t)/9}\n\t\t)\n\t)\n} * -20.dbamp\n).fadeTime_(6).play;\n)\n\nNdef(\\c).clear(6);\n\n\n/////// Some one-liners by Fredrik Olofsson (redFrik):\n\nplay{a={|r,i,n,q,p=0|ar(RLPF,ar(Impulse,r,p),kr(Select,i%5,[0,3,5,7,10]+n).midicps,q)};o=ar(Splay,[a.(2,ar(l=LFSaw,x=1e-3)*88,51,x*9),a.(1,0,70,x*5,b=(1..8)/8),a.(ar(l,1/9)+1,5**ar(l,b/9),82,x*3,b*3)/8,a.(3,ar(l,x*2,0.5)*88,63,x*7)/4].sum);o+ar(GVerb,o.sum/9,50)}\n\n\nplay{ar(Splay,{|i|ar(CombC,ar(a=SinOsc,Scale.yu.degreeToFreq(\\fredaglunch.ascii%12@i,ar(a,1/16)>0*2+6-(ar(a,ar(a,1/32,i))/20),5))/9,0.5,i+floor(ar(a,1/64)*6+6).varlag(0.1)%11/33+0.1+varlag(ar(a,ar(a,1/4,i/11*pi)<0*2)*0.1,0.01,ar(a,0.01,i)),ar(a,1/9)+1)}!11).tanh}\n\n\nplay{tanh(ar(a=SinOscFB,ar(a,1/64)<0*9+50,ar(a,1/8)+1/2,ar(HPF,c=ar(a,3)>0,4,5))!2+ar(Splay,ar(a,ar(Demand,ar(a,1/12),0,Dseq([1,1,2,3,4,1]*(ar(a,1/96)<0.5*19+99),inf)*b=[1,0.5,1.25]).lag3ud(1,3),ar(a,b/16)+1/3))+(ar(HPF,c>0.1,9**ar(a,1/19)+3)*ar(a,[12,9]*99)/6))}\n\na=LFCub;play{RecordBuf.ar(InFeedback.ar+a.ar(99),b=Buffer.alloc(s,8e4));TGrains.ar(2,a ar:c=[3,2],b,a.ar(1/c)>0/2+1.5,0,3)}\n\n\n\n///////////////////////////////\n\n\n\n\n////////////////////// BUFFERS\n// drag&drop sound files from finder. filepath will be printed.\n\n(\n~buffers= [\n    \"/Users/bleh/Documents/sample1.aiff\", //edit here to match your own soundfiles\n    \"/Users/bleh/Documents/sample2.aiff\",\n    \"/Users/bleh/Documents/sample3.aiff\"\n    //add more buffers here\n].collect{|path| Buffer.readChannel(s, path, channels:[0])};\nSynthDef(\\sampler, {|buf, atk= 5, sus= 5, rel= 5, rate= 1|\n    var env= EnvGen.kr(Env.linen(atk, sus, rel), doneAction:2);\n    var src= PlayBuf.ar(1, buf, rate, loop: 1);\n    Out.ar(0, Pan2.ar(src, LFNoise2.kr(0.1), env));\n}).add;\n)\n\n(\nTdef(\\player, {\n    var atk, sus, rel, rate;\n    loop{\n        atk= 3.rrand(6); //attack time\n        sus= 5.rrand(10); //sustain time\n        rel= 8.rrand(10); //release time\n        rate= 0.15.bilinrand+1; //variation in pitch (playback speed)\n        Synth(\\sampler, [\\buf, ~buffers.choose, \\atk, atk, \\sus, sus, \\rel, rel, \\rate, rate, ]);\n        5.rrand(8).wait; //time between new synths\n    };\n}).play;\n)\n\nTdef(\\player).stop;\n\n///////////////////////////////\n////Max Console = Supercollider Post Window\n//-->\n\n( \nvar t;\nq=[\n\t[[\" \",\"│\",\" \"],[\"╮\",\"╰\",\"┬\"],[\"╰\",\"┬\",\"╯\"]],\n\t[[\"╭\",\"╯\",\"◦\"],[\"┴\",\"┬\",\"─\"],[\" \",\"│\",\" \"]],\n\t[[\"o\",\"│\",\" \"],[\"─\",\"╯\",\"╭\"],[\" \",\"╭\",\"╯\"]],\n\t[[\" \",\"╰\",\"╮\"],[\"╮\",\" \",\"╰\"],[\"╰\",\"╮\",\" \"]],\n\t[[\"╭\",\"┴\",\"╮\"],[\"┤\",\"°\",\"╰\"],[\"╰\",\"╮\",\"◯\"]],\n\t[[\"╭\",\"┴\",\"╮\"],[\"┤\",\"O\",\"├\"],[\"╰\",\"┬\",\"╯\"]],\n];\n8.do{\n\tt=q.pyramid(3).scramble;\n\tt[0].size.do{|l|\n\t\tt.size.do{|n|t[0].size.do{|i|t[n][l][i].post}};\n\tPost.nl\n\t}\n});\n\n/////////////////////////////\n\nsnip.SnippetKey // See repo's README for snippets.\n\n\n\n",
                    "scrollspeed": 6.0,
                    "snippets": ""
                }
            },
            {
                "box": {
                    "id": "obj-16",
                    "linecount": 13,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 190.0, 184.0, 145.0, 194.0 ],
                    "text": "<--To stop any running code, use the Ctrl+. shortcut (Ctrl+Period). This will stop all the code scheduled on the clocks, as well as all the sound processes on the audio server.\n\nDO NOT press Cmd+Period. It stops Max Scheduler.\n\n"
                }
            },
            {
                "box": {
                    "id": "obj-14",
                    "linecount": 2,
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [ 40.25, 19.0, 199.0, 33.0 ],
                    "text": "1 - Boot the engine (bang or open)  Check the Max console for errors"
                }
            },
            {
                "box": {
                    "id": "obj-12",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 227.0, 377.0, 29.5, 22.0 ],
                    "text": "info"
                }
            },
            {
                "box": {
                    "bgcolor": [ 0.9254901960784314, 0.09411764705882353, 0.09411764705882353, 1.0 ],
                    "bgcolor2": [ 0.17533612107834648, 0.17533607062555467, 0.1753360838096752, 1.0 ],
                    "bgfillcolor_angle": 270.0,
                    "bgfillcolor_autogradient": 0.0,
                    "bgfillcolor_color": [ 0.9254901960784314, 0.09411764705882353, 0.09411764705882353, 1.0 ],
                    "bgfillcolor_color1": [ 0.9254901960784314, 0.09411764705882353, 0.09411764705882353, 1.0 ],
                    "bgfillcolor_color2": [ 0.17533612107834648, 0.17533607062555467, 0.1753360838096752, 1.0 ],
                    "bgfillcolor_proportion": 0.5,
                    "bgfillcolor_type": "gradient",
                    "gradient": 1,
                    "id": "obj-10",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 149.0, 184.0, 37.0, 22.0 ],
                    "text": "panic"
                }
            },
            {
                "box": {
                    "id": "obj-8",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 178.0, 377.0, 37.0, 22.0 ],
                    "text": "close"
                }
            },
            {
                "box": {
                    "id": "obj-6",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "outlettype": [ "" ],
                    "patching_rect": [ 112.0, 61.0, 35.0, 22.0 ],
                    "text": "open"
                }
            },
            {
                "box": {
                    "id": "obj-4",
                    "maxclass": "button",
                    "numinlets": 1,
                    "numoutlets": 1,
                    "outlettype": [ "bang" ],
                    "parameter_enable": 0,
                    "patching_rect": [ 73.0, 60.0, 24.0, 24.0 ]
                }
            },
            {
                "box": {
                    "id": "obj-1",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 4,
                    "outlettype": [ "", "", "signal", "signal" ],
                    "patching_rect": [ 91.0, 532.0, 97.0, 22.0 ],
                    "text": "sc3~"
                }
            }
        ],
        "lines": [
            {
                "patchline": {
                    "destination": [ "obj-20", 1 ],
                    "source": [ "obj-1", 3 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-20", 0 ],
                    "source": [ "obj-1", 2 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-1", 0 ],
                    "midpoints": [ 158.5, 519.0, 100.5, 519.0 ],
                    "source": [ "obj-10", 0 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-1", 0 ],
                    "midpoints": [ 236.5, 519.0, 100.5, 519.0 ],
                    "source": [ "obj-12", 0 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-1", 0 ],
                    "midpoints": [ 371.5, 519.0, 100.5, 519.0 ],
                    "source": [ "obj-17", 0 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-3", 1 ],
                    "source": [ "obj-2", 3 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-3", 0 ],
                    "source": [ "obj-2", 2 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-19", 1 ],
                    "source": [ "obj-20", 1 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-19", 0 ],
                    "source": [ "obj-20", 0 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-1", 0 ],
                    "source": [ "obj-21", 0 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-35", 0 ],
                    "source": [ "obj-22", 0 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-26", 0 ],
                    "source": [ "obj-28", 0 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-22", 0 ],
                    "source": [ "obj-29", 0 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-19", 1 ],
                    "midpoints": [ 376.75, 1017.0, 195.0, 1017.0, 195.0, 816.0, 173.5, 816.0 ],
                    "source": [ "obj-3", 1 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-19", 0 ],
                    "midpoints": [ 369.5, 1017.0, 195.0, 1017.0, 195.0, 807.0, 147.5, 807.0 ],
                    "source": [ "obj-3", 0 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-23", 0 ],
                    "source": [ "obj-30", 0 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-2", 0 ],
                    "midpoints": [ 483.5, 826.0625, 312.5, 826.0625 ],
                    "source": [ "obj-33", 0 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-29", 0 ],
                    "source": [ "obj-34", 0 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-1", 0 ],
                    "midpoints": [ 82.5, 519.0, 100.5, 519.0 ],
                    "source": [ "obj-4", 0 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-17", 0 ],
                    "midpoints": [ 971.5, 153.0, 371.5, 153.0 ],
                    "source": [ "obj-5", 0 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-1", 0 ],
                    "midpoints": [ 121.5, 519.0, 100.5, 519.0 ],
                    "source": [ "obj-6", 0 ]
                }
            },
            {
                "patchline": {
                    "destination": [ "obj-1", 0 ],
                    "midpoints": [ 187.5, 519.0, 100.5, 519.0 ],
                    "source": [ "obj-8", 0 ]
                }
            }
        ],
        "parameters": {
            "obj-20": [ "live.gain~", "live.gain~", 0 ],
            "obj-28": [ "number", "number", 0 ],
            "obj-3": [ "live.gain~[1]", "live.gain~[1]", 0 ],
            "obj-30": [ "number[1]", "number[1]", 0 ],
            "parameterbanks": {
                "0": {
                    "index": 0,
                    "name": "",
                    "parameters": [ "-", "-", "-", "-", "-", "-", "-", "-" ],
                    "buttons": [ "-", "-", "-", "-", "-", "-", "-", "-" ]
                }
            },
            "inherited_shortname": 1
        },
        "autosave": 0
    }
}