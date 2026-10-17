/*
 * Monster Hunter Stories (Android 1.0.5) -> PS Vita
 *
 * Implementations for the extra imports listed in dynlib_extra.inc.
 */

#include "mhs/dynlib_extra.h"

#include <stdarg.h>
#include <stdio.h>

#include "utils/dialog.h"
#include "utils/logger.h"

void mhs_noop(void) {}

void mhs_android_log_assert(const char *cond, const char *tag, const char *fmt, ...) {
    char buf[512];
    va_list va;
    va_start(va, fmt);
    vsnprintf(buf, sizeof(buf), fmt ? fmt : "(no message)", va);
    va_end(va);
    l_debug("[%s] assert %s: %s", tag ? tag : "?", cond ? cond : "", buf);
    fatal_error("Game assertion failed:\n%s", buf);
}

static const char dummy_effectsend[16] = "MHS_EFFECTSEND";
static const char dummy_reverb[16] = "MHS_ENVREVERB";
const void *mhs_SL_IID_EFFECTSEND = dummy_effectsend;
const void *mhs_SL_IID_ENVIRONMENTALREVERB = dummy_reverb;
