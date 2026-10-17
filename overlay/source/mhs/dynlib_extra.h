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

#endif
