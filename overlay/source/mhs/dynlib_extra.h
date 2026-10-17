/*
 * Monster Hunter Stories (Android 1.0.5) -> PS Vita
 *
 * libMHR.so imports that soloader-boilerplate's dynlib.c may not cover.
 * setup.sh injects this header at the top of source/dynlib.c and
 * dynlib_extra.inc at the start of the default_dynlib[] table, so these
 * entries take priority over the boilerplate's.
 */

#ifndef MHS_DYNLIB_EXTRA_H
#define MHS_DYNLIB_EXTRA_H

#include <stdint.h>

void mhs_noop(void);
void mhs_android_log_assert(const char *cond, const char *tag, const char *fmt, ...);

/* OpenSL ES interfaces the game asks for but the Vita OpenSL wrapper may not
 * implement (reverb/effect send). Unique dummy IDs make GetInterface() fail
 * cleanly instead of resolving to NULL. */
extern const void *mhs_SL_IID_EFFECTSEND;
extern const void *mhs_SL_IID_ENVIRONMENTALREVERB;

/* ---- Diagnostics -------------------------------------------------------
 * The game's own logging (native::debug::trace) is compiled to empty stubs and
 * its fatal paths end in a silent exit(). These route both to
 * DATA_PATH "mhs_log.txt" and turn fatal exits into an on-screen dialog. */

extern uintptr_t mhs_libmhr_base;   /* set by patch.c */

void mhs_log(const char *fmt, ...);
void mhs_exit(int code);
void mhs_abort(void);

/* Hook targets installed by patch.c (signatures match the game's functions). */
void mhs_hook_trace(int type, const char *fmt, ...);
void mhs_hook_trace_direct(int type, const char *msg);
void mhs_hook_handler(const char *msg, void *ctx);
void mhs_hook_mtdebug_abort(void *self, const char *fmt, ...);
void mhs_hook_abort_program(const char *fmt, ...);

#endif
