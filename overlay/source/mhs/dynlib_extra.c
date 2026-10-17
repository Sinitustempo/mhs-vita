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

/* ---- Diagnostics ------------------------------------------------------- */

#include <psp2/io/fcntl.h>
#include <psp2/kernel/clib.h>
#include <string.h>

uintptr_t mhs_libmhr_base;

static SceUID log_fd = -1;
static char last_line[256];

static void log_write(const char *s) {
    if (log_fd < 0) {
        log_fd = sceIoOpen(DATA_PATH "mhs_log.txt", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
        if (log_fd < 0) return;
    }
    size_t n = strlen(s);
    sceIoWrite(log_fd, s, n);
    if (!n || s[n - 1] != '\n') sceIoWrite(log_fd, "\n", 1);
}

static void log_va(const char *prefix, const char *fmt, va_list va) {
    char buf[512];
    int p = sceClibSnprintf(buf, sizeof(buf), "%s", prefix);
    vsnprintf(buf + p, sizeof(buf) - p, fmt ? fmt : "(null)", va);
    strncpy(last_line, buf, sizeof(last_line) - 1);
    log_write(buf);
}

void mhs_log(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    log_va("", fmt, va);
    va_end(va);
}

/* Where in libMHR.so did a call come from (for the dialog / log). */
static uintptr_t mhr_off(void *ret) {
    uintptr_t a = (uintptr_t)ret;
    return (mhs_libmhr_base && a >= mhs_libmhr_base) ? a - mhs_libmhr_base : a;
}

void mhs_exit(int code) {
    uintptr_t from = mhr_off(__builtin_return_address(0));
    mhs_log("[exit] code %d from libMHR+0x%x", code, from);
    fatal_error("The game quit (exit %d) from libMHR+0x%x.\n\nLast message:\n%s\n\nFull log: "
                DATA_PATH "mhs_log.txt", code, from, last_line);
}

void mhs_abort(void) {
    uintptr_t from = mhr_off(__builtin_return_address(0));
    mhs_log("[abort] from libMHR+0x%x", from);
    fatal_error("The game aborted from libMHR+0x%x.\n\nLast message:\n%s\n\nFull log: "
                DATA_PATH "mhs_log.txt", from, last_line);
}

void mhs_hook_trace(int type, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    log_va("[trace] ", fmt, va);
    va_end(va);
}

void mhs_hook_trace_direct(int type, const char *msg) {
    mhs_log("[trace] %s", msg ? msg : "(null)");
}

void mhs_hook_handler(const char *msg, void *ctx) {
    mhs_log("[mt] %s", msg ? msg : "(null)");
}

void mhs_hook_mtdebug_abort(void *self, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    log_va("[MtDebug::abort] ", fmt, va);
    va_end(va);
    fatal_error("Game error (MtDebug::abort):\n%s\n\nFull log: " DATA_PATH "mhs_log.txt", last_line);
}

void mhs_hook_abort_program(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    log_va("[abortProgram] ", fmt, va);
    va_end(va);
    fatal_error("Game error (abortProgram):\n%s\n\nFull log: " DATA_PATH "mhs_log.txt", last_line);
}

static const char dummy_effectsend[16] = "MHS_EFFECTSEND";
static const char dummy_reverb[16] = "MHS_ENVREVERB";
const void *mhs_SL_IID_EFFECTSEND = dummy_effectsend;
const void *mhs_SL_IID_ENVIRONMENTALREVERB = dummy_reverb;
