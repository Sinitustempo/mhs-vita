/*
 * Monster Hunter Stories (Android 1.0.5) -> PS Vita
 *
 * Implementations for the extra imports listed in dynlib_extra.inc.
 */

#include "mhs/dynlib_extra.h"

#include <errno.h>
#include <reent.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <psp2/kernel/rng.h>

#include "utils/dialog.h"
#include "utils/logger.h"

/* Recent VitaSDK newlib builds have getentropy() call _getentropy_r() without
 * providing it, which breaks linking. Back it with the hardware RNG. */
int _getentropy_r(struct _reent *r, void *buf, size_t len) {
    if (len > 256) {
        r->_errno = EIO;
        return -1;
    }
    uint8_t *p = buf;
    while (len) {
        size_t n = len > 64 ? 64 : len;    /* sceKernelGetRandomNumber caps at 64 bytes */
        if (sceKernelGetRandomNumber(p, n) < 0) {
            r->_errno = EIO;
            return -1;
        }
        p += n;
        len -= n;
    }
    return 0;
}

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
