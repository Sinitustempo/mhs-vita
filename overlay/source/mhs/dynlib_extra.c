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

/* Boilerplate implementations we wrap (source/reimpl/io.c, asset_manager.cpp, libc). */
extern int   open_soloader(const char *path, int flags, int mode);
extern void *fopen_soloader(const char *path, const char *mode);
extern int   stat_soloader(const char *path, void *st);
extern void *opendir_soloader(const char *path);
extern int   access(const char *path, int mode);
extern void *AAssetManager_open(void *mgr, const char *name, int mode);

extern int   fstat_soloader(int fd, void *st);
extern int   close_soloader(int fd);
extern int   read(int fd, void *buf, unsigned n);
extern long  lseek(int fd, long off, int whence);

/* fds of open .obb files: reads on these get logged (bounded). */
#define MAX_TRACKED 8
static int tracked_fd[MAX_TRACKED] = { -1, -1, -1, -1, -1, -1, -1, -1 };
static int tracked_logs;

static int is_tracked(int fd) {
    for (int i = 0; i < MAX_TRACKED; i++) if (tracked_fd[i] == fd && fd >= 0) return 1;
    return 0;
}

int mhs_open(const char *path, int flags, int mode) {
    int r = open_soloader(path, flags, mode);
    mhs_log("[io] open(%s, 0x%x) = %d", path ? path : "(null)", flags, r);
    size_t n = path ? strlen(path) : 0;
    if (r >= 0 && n > 4 && strcmp(path + n - 4, ".obb") == 0) {
        for (int i = 0; i < MAX_TRACKED; i++) if (tracked_fd[i] < 0) { tracked_fd[i] = r; break; }
    }
    return r;
}

int mhs_read(int fd, void *buf, unsigned n) {
    int r = read(fd, buf, n);
    if (is_tracked(fd) && tracked_logs++ < 200) {
        const unsigned char *b = buf;
        mhs_log("[io] read(fd %d, %u) = %d  first bytes %02x %02x %02x %02x", fd, n, r,
                r > 0 ? b[0] : 0, r > 1 ? b[1] : 0, r > 2 ? b[2] : 0, r > 3 ? b[3] : 0);
    }
    return r;
}

long mhs_lseek(int fd, long off, int whence) {
    long r = lseek(fd, off, whence);
    if (is_tracked(fd) && tracked_logs++ < 200)
        mhs_log("[io] lseek(fd %d, %ld, %d) = %ld", fd, off, whence, r);
    return r;
}

int mhs_fstat(int fd, void *st) {
    int r = fstat_soloader(fd, st);
    if (is_tracked(fd)) {
        /* bionic arm stat: st_size is a 64-bit field at offset 0x30 */
        long long sz = *(long long *)((char *)st + 0x30);
        mhs_log("[io] fstat(fd %d) = %d  st_size %lld", fd, r, sz);
    }
    return r;
}

int mhs_close(int fd) {
    if (is_tracked(fd)) {
        mhs_log("[io] close(fd %d)", fd);
        for (int i = 0; i < MAX_TRACKED; i++) if (tracked_fd[i] == fd) tracked_fd[i] = -1;
    }
    return close_soloader(fd);
}

void *mhs_fopen(const char *path, const char *mode) {
    void *r = fopen_soloader(path, mode);
    mhs_log("[io] fopen(%s, %s) = %p", path ? path : "(null)", mode ? mode : "", r);
    return r;
}

int mhs_stat(const char *path, void *st) {
    int r = stat_soloader(path, st);
    mhs_log("[io] stat(%s) = %d", path ? path : "(null)", r);
    return r;
}

void *mhs_opendir(const char *path) {
    void *r = opendir_soloader(path);
    mhs_log("[io] opendir(%s) = %p", path ? path : "(null)", r);
    return r;
}

int mhs_access(const char *path, int mode) {
    int r = access(path, mode);
    mhs_log("[io] access(%s, %d) = %d", path ? path : "(null)", mode, r);
    return r;
}

void *mhs_AAssetManager_open(void *mgr, const char *name, int mode) {
    void *r = AAssetManager_open(mgr, name, mode);
    mhs_log("[io] AAssetManager_open(%s) = %p", name ? name : "(null)", r);
    return r;
}

static const char dummy_effectsend[16] = "MHS_EFFECTSEND";
static const char dummy_reverb[16] = "MHS_ENVREVERB";
const void *mhs_SL_IID_EFFECTSEND = dummy_effectsend;
const void *mhs_SL_IID_ENVIRONMENTALREVERB = dummy_reverb;
