/* keylog — K0 de `sc_code`: spike que sólo MIDE qué teclas entrega Max.
 *
 * Objetivo (the design notes riesgo nº1): averiguar si Max entrega las teclas con ⌘ a
 * `keyfilter` / `key`. De eso dependen ⌘Return, ⌘C/V/X/A/Z, ⇧⌘Z y ⌘. en el editor
 * in-patch, y el fallback a ⌥/⌃ si no llegan.
 *
 * Qué hace:
 *   - jbox mínimo (sin SC, sin MSP, sin textfield), `paint` muestra el último evento.
 *   - métodos `key` y `keyfilter`: loguean keycode / modifiers (hex + decodificado
 *     cmd/shift/alt/ctrl/auto) / textcharacter a /tmp/sc_code_keylog.txt y con
 *     `object_post` a la consola de Max.
 *   - `focusgained`/`focuslost`/`mousedown`: para distinguir "no llega la tecla" de
 *     "no tiene foco".
 *   - Devuelve 0 en `keyfilter` y en `key` (deja pasar todo) ⇒ el spike es
 *     transparente y no cambia lo que Max haría con ⌘. o ⌘E.
 *
 * NO se distribuye: es código de medición, no parte de `sc_code`.
 *
 * Compila como .mxo suelto (arm64, minos 11, -bundle -undefined dynamic_lookup):
 * todos los símbolos de Max los resuelve el host al cargar.
 */

#include "ext.h"
#include "ext_obex.h"
#include "jpatcher_api.h"
#include "jpatcher_syms.h" /* jpatcher_syms_init(): SIN esto jbox_new() segfaultea (K0,) */
#include "jgraphics.h"

#include <cstdio>
#include <cstring>
#include <ctime>

#define KEYLOG_VERSION "0.1.0"
#define KEYLOG_PATH "/tmp/sc_code_keylog.txt"

typedef struct _keylog {
	t_jbox j_box;        /* debe ser el primer campo */
	long counter;        /* nº de eventos vistos */
	long last_src;       /* 0 = ninguno, 1 = keyfilter, 2 = key */
	long last_kc;
	long last_mod;
	long last_tc;
	long focused;
	char last_mods[64];  /* "cmd|shift" */
	char last_text[256]; /* resumen para `paint` */
} t_keylog;

static t_class *s_keylog_class = NULL;
static FILE *s_log = NULL; /* se abre una vez (ext_main) y se cierra con fflush */

/**********************************************************************/
/* logging                                                             */

/* Decodifica el bitmask de modificadores de Max (max_keydefs.h: t_modifiers). */
static void keylog_decode_mods(long mods, char *out, size_t outlen) {
	out[0] = 0;
	static const struct {
		long bit;
		const char *name;
	} names[] = {
		{ eCommandKey, "cmd" }, { eShiftKey, "shift" }, { eControlKey, "ctrl" },
		{ eAltKey, "alt" },     { eAutoRepeat, "auto" }, { eCapsLock, "caps" },
		{ eLeftButton, "Lbtn" }, { eRightButton, "Rbtn" }, { eMiddleButton, "Mbtn" },
		{ ePopupMenu, "popup" },
	};
	for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
		if (mods & names[i].bit) {
			if (out[0])
				strncat(out, "|", outlen - strlen(out) - 1);
			strncat(out, names[i].name, outlen - strlen(out) - 1);
		}
	}
	if (!out[0])
		strncpy(out, "-", outlen - 1);
}

/* textcharacter → texto legible (puede venir 0 para teclas especiales). */
static void keylog_decode_text(long tc, char *out, size_t outlen) {
	switch (tc) {
	case 0:
		snprintf(out, outlen, "0x0 (0)");
		break;
	case 13:
		snprintf(out, outlen, "0xd (13) \\r");
		break;
	case 9:
		snprintf(out, outlen, "0x9 (9) \\t");
		break;
	case 8:
		snprintf(out, outlen, "0x8 (8) \\b");
		break;
	case 27:
		snprintf(out, outlen, "0x1b (27) ESC");
		break;
	case 127:
		snprintf(out, outlen, "0x7f (127) DEL");
		break;
	default:
		if (tc > 32 && tc < 127)
			snprintf(out, outlen, "0x%lx (%ld) '%c'", tc, tc, (char)tc);
		else
			snprintf(out, outlen, "0x%lx (%ld)", tc, tc);
		break;
	}
}

static long keylog_keycode(long kc) {
	/* Los JKEY_* son negativos; fuera de eso Max entrega el ASCII sin modificadores. */
	return kc;
}

static void keylog_record(t_keylog *x, const char *src, long kc, long mods, long tc) {
	char modsText[64], textText[64];
	keylog_decode_mods(mods, modsText, sizeof(modsText));
	keylog_decode_text(tc, textText, sizeof(textText));

	x->counter++;
	x->last_src = (strcmp(src, "keyfilter") == 0) ? 1 : 2;
	x->last_kc = kc;
	x->last_mod = mods;
	x->last_tc = tc;
	strncpy(x->last_mods, modsText, sizeof(x->last_mods) - 1);

	snprintf(x->last_text, sizeof(x->last_text), "%s kc=%ld mod=0x%lx [%s] tc=%s", src, keylog_keycode(kc),
			 mods, modsText, textText);

	if (s_log) {
		fprintf(s_log, "%4ld  %-9s kc=%6ld  mod=0x%05lx [%s]  tc=%s\n", x->counter, src, kc, mods,
				modsText, textText);
		fflush(s_log);
	}
	object_post((t_object *)x, "%s", x->last_text);
	jbox_redraw(&x->j_box);
}

/**********************************************************************/
/* life cycle                                                          */

static t_keylog *keylog_new(t_symbol *s, long argc, t_atom *argv);
static void keylog_free(t_keylog *x);

static void keylog_paint(t_keylog *x, t_object *view);
static long keylog_key(t_keylog *x, t_object *patcherview, long keycode, long modifiers, long textcharacter);
static long keylog_keyfilter(t_keylog *x, t_object *patcherview, long *keycode, long *modifiers,
							 long *textcharacter);
static void keylog_focusgained(t_keylog *x, t_object *patcherview);
static void keylog_focuslost(t_keylog *x, t_object *patcherview);
static void keylog_mousedown(t_keylog *x, t_object *patcherview, t_pt pt, long modifiers);
static void keylog_getdrawparams(t_keylog *x, t_object *patcherview, t_jboxdrawparams *params);
static void keylog_bang(t_keylog *x);

C74_EXPORT void ext_main(void *r) {
	t_class *c;

	/* Prerrequisito de la SDK (no documentado en el header de jbox): llena las
	 * tablas de t_symbol* estáticas que usa jbox_new(). Sin esta llamada, la mera
	 * creación de la caja tira SIGSEGV dentro de jbox_new (+0x110 desde NULL) y se
	 * lleva Max puesto. Requiere compilar .../c74support/max-includes/common/
	 * jpatcher_syms.c dentro del bundle (ver Makefile). */
	jpatcher_syms_init();

	/* Log fresco por cada carga del bundle (cada arranque de Max). */
	s_log = fopen(KEYLOG_PATH, "w");
	if (s_log) {
		time_t now = time(NULL);
		char stamp[64];
		struct tm tmv;
		localtime_r(&now, &tmv);
		strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &tmv);
		fprintf(s_log, "# keylog %s — %s\n", KEYLOG_VERSION, stamp);
		fprintf(s_log, "# %4s  %-9s %-14s %-24s %s\n", "n", "method", "keycode", "modifiers", "textcharacter");
		fflush(s_log);
	}

	c = class_new("keylog", (method)keylog_new, (method)keylog_free, sizeof(t_keylog), (method)NULL, A_GIMME, 0L);

	c->c_flags |= CLASS_FLAG_NEWDICTIONARY; /* objeto de UI: Max pasa un dictionary (ver jbox_new) */
	jbox_initclass(c, 0);

	class_addmethod(c, (method)keylog_paint, "paint", A_CANT, 0);
	class_addmethod(c, (method)keylog_key, "key", A_CANT, 0);
	class_addmethod(c, (method)keylog_keyfilter, "keyfilter", A_CANT, 0);
	class_addmethod(c, (method)keylog_focusgained, "focusgained", A_CANT, 0);
	class_addmethod(c, (method)keylog_focuslost, "focuslost", A_CANT, 0);
	class_addmethod(c, (method)keylog_mousedown, "mousedown", A_CANT, 0);
	class_addmethod(c, (method)keylog_getdrawparams, "getdrawparams", A_CANT, 0);
	class_addmethod(c, (method)keylog_bang, "bang", 0);

	CLASS_ATTR_DEFAULT(c, "rect", 0, "0. 0. 340. 90.");

	class_register(CLASS_BOX, c);
	s_keylog_class = c;
}

static t_keylog *keylog_new(t_symbol *s, long argc, t_atom *argv) {
	t_dictionary *d = NULL;
	t_keylog *x = NULL;

	/* Objetos de UI: el argumento es un dictionary de atributos (CLASS_FLAG_NEWDICTIONARY).
	 * Sin este chequeo, Max entrega argc/argv crudos y jbox_new() revienta. */
	if (!(d = object_dictionaryarg(argc, argv)))
		return NULL;

	x = (t_keylog *)object_alloc(s_keylog_class);

	if (x) {
		long flags = 0
					 | JBOX_DRAWFIRSTIN
					 | JBOX_TRANSPARENT
					 | JBOX_DRAWINLAST
					 | JBOX_HILITE /* acepta foco al clickear: sin foco no hay teclas */
			;
		jbox_new(&x->j_box, flags, argc, argv);
		x->j_box.b_firstin = (t_object *)x;
		x->counter = 0;
		x->last_src = 0;
		x->last_kc = x->last_mod = x->last_tc = 0;
		x->focused = 0;
		strncpy(x->last_mods, "-", sizeof(x->last_mods));
		snprintf(x->last_text, sizeof(x->last_text), "sin eventos — clickeá la caja y apretá teclas");
		attr_dictionary_process((t_object *)x, d); /* atributos de la caja (rect, …) */
		jbox_ready(&x->j_box);
	}
	return x;
}

static void keylog_free(t_keylog *x) {
	jbox_free(&x->j_box);
}

/**********************************************************************/
/* paint                                                               */

static void keylog_paint(t_keylog *x, t_object *view) {
	t_jgraphics *g = (t_jgraphics *)patcherview_get_jgraphics(view);
	t_rect rect;
	jbox_get_rect_for_view((t_object *)x, view, &rect);

	/* fondo */
	jgraphics_rectangle(g, 0., 0., rect.width, rect.height);
	jgraphics_set_source_rgba(g, x->focused ? 0.10 : 0.16, 0.10, 0.14, 1.0);
	jgraphics_fill(g);

	jgraphics_select_font_face(g, "Menlo", JGRAPHICS_FONT_SLANT_NORMAL, JGRAPHICS_FONT_WEIGHT_NORMAL);
	jgraphics_set_font_size(g, 10.);

	/* línea 1: estado */
	char line[320];
	snprintf(line, sizeof(line), "keylog %s  #%ld  focus=%s", KEYLOG_VERSION, x->counter,
			 x->focused ? "SI" : "NO (clickeá la caja)");
	jgraphics_set_source_rgba(g, 0.65, 0.70, 0.75, 1.0);
	jgraphics_move_to(g, 8., 16.);
	jgraphics_show_text(g, line);

	/* línea 2: último evento */
	jgraphics_set_source_rgba(g, 0.95, 0.95, 0.98, 1.0);
	jgraphics_move_to(g, 8., 36.);
	jgraphics_show_text(g, x->last_src ? x->last_text : "—");

	/* línea 3: crudo en hex, para copiar a mano si hace falta */
	char raw[128];
	snprintf(raw, sizeof(raw), "kc=%ld mod=0x%lx tc=%ld (%ld)", x->last_kc, x->last_mod, x->last_tc,
			 x->counter);
	jgraphics_set_source_rgba(g, 0.55, 0.60, 0.65, 1.0);
	jgraphics_move_to(g, 8., 56.);
	jgraphics_show_text(g, raw);
}

static void keylog_getdrawparams(t_keylog *x, t_object *patcherview, t_jboxdrawparams *params) {
	if (x->focused) {
		params->d_bordercolor.red = 1.0;
		params->d_bordercolor.green = 0.4;
		params->d_bordercolor.blue = 0.1;
		params->d_bordercolor.alpha = 1.0;
		params->d_borderthickness = 2.;
	}
}

/**********************************************************************/
/* teclas                                                              */

static long keylog_key(t_keylog *x, t_object *patcherview, long keycode, long modifiers, long textcharacter) {
	keylog_record(x, "key", keycode, modifiers, textcharacter);
	return 0; /* transparencia: no consumimos nada */
}

static long keylog_keyfilter(t_keylog *x, t_object *patcherview, long *keycode, long *modifiers,
							 long *textcharacter) {
	keylog_record(x, "keyfilter", *keycode, *modifiers, *textcharacter);
	return 0; /* transparencia: dejamos pasar la tecla */
}

static void keylog_focusgained(t_keylog *x, t_object *patcherview) {
	x->focused = 1;
	if (s_log) {
		fprintf(s_log, "%4s  focus     gained\n", "·");
		fflush(s_log);
	}
	object_post((t_object *)x, "focus gained");
	jbox_redraw(&x->j_box);
}

static void keylog_focuslost(t_keylog *x, t_object *patcherview) {
	x->focused = 0;
	if (s_log) {
		fprintf(s_log, "%4s  focus     lost\n", "·");
		fflush(s_log);
	}
	object_post((t_object *)x, "focus lost");
	jbox_redraw(&x->j_box);
}

static void keylog_mousedown(t_keylog *x, t_object *patcherview, t_pt pt, long modifiers) {
	if (s_log) {
		fprintf(s_log, "%4s  mousedown\n", "·");
		fflush(s_log);
	}
}

static void keylog_bang(t_keylog *x) {
	if (s_log) {
		fprintf(s_log, "%4s  -------- marker --------\n", "·");
		fflush(s_log);
	}
	object_post((t_object *)x, "marker");
	jbox_redraw(&x->j_box);
}
