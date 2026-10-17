/*
 * Monster Hunter Stories (Android 1.0.5) -> PS Vita
 *
 * Replaces soloader-boilerplate's source/patch.c. The kuser helper patch is
 * kept from the boilerplate (Android's __kuser_cmpxchg / memory barrier live
 * at fixed addresses that don't exist on the Vita).
 *
 * libMHR.so ships with full C++ symbols (~106k exports), so game hooks can be
 * placed by name with so_symbol() instead of raw offsets.
 */

#include <kubridge.h>
#include <so_util/so_util.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>
#include <vitasdk.h>

extern so_module so_mod;

#define SCE_KERNEL_MEMBLOCK_TYPE_USER_RX (0x0C20D050)

#include "utils/logger.h"
#include "utils/dialog.h"
#include "reimpl/sys.h"
#include "mhs/dynlib_extra.h"

void __kuser_memory_barrier(void) {
    __sync_synchronize();
}

static void kuser_patch(void) {
    SceKernelAllocMemBlockKernelOpt opt;
    memset(&opt, 0, sizeof(SceKernelAllocMemBlockKernelOpt));
    opt.size = sizeof(SceKernelAllocMemBlockKernelOpt);
    opt.attr = 0x1;
    opt.field_C = (SceUInt32)0x9A000000;
    if (kuKernelAllocMemBlock("atomic", SCE_KERNEL_MEMBLOCK_TYPE_USER_RX, 0x1000, &opt) < 0)
        fatal_error("Error could not allocate atomic block.");
    kuKernelMemProtect((void *)0x9A000000, (SceSize)0x1000, KU_KERNEL_PROT_EXEC | KU_KERNEL_PROT_READ | KU_KERNEL_PROT_WRITE);

    hook_addr(0x9A000FA0, (uintptr_t)__kuser_memory_barrier);
    hook_addr(0x9A000FC0, (uintptr_t)__atomic_cmpxchg);

    uint32_t patched_addr;
    for (uint32_t addr = so_mod.text_base; addr < so_mod.text_base + so_mod.text_size; addr += 4) {
        uint32_t *a = (uint32_t *)addr;
        if (*a == 0xFFFF0FC0) {
            patched_addr = 0x9A000FC0;
            kuKernelCpuUnrestrictedMemcpy((void *)(addr), &patched_addr, sizeof(uint32_t));
        } else if (*a == 0xFFFF0FA0) {
            patched_addr = 0x9A000FA0;
            kuKernelCpuUnrestrictedMemcpy((void *)(addr), &patched_addr, sizeof(uint32_t));
        }
    }
}

/* Hook a libMHR.so function by its mangled name, logging if it isn't found. */
static void hook_sym(const char *sym, uintptr_t dst) {
    uintptr_t addr = so_symbol(&so_mod, sym);
    if (!addr) {
        l_debug("patch: symbol not found: %s", sym);
        return;
    }
    hook_addr(addr, dst);
}

/* ---- Game patches ------------------------------------------------------ */

/* MT Framework's default heaps (MtMemoryAllocator::allocateDefaultMemory).
 * Each MtScalableAllocator reserves its whole size up front, ~730 MB in total.
 * That's fine with Android's lazily-committed virtual memory but impossible on
 * the Vita: the 216 MB Resource heap failed to allocate, leaving that allocator
 * empty, and sResource's constructor crashed on its first (20 byte) allocation.
 *
 * The table is 9 x 24-byte entries {name, index, type, size, attr, pad} at this
 * offset in libMHR.so 1.0.5. Sizes below are a first guess at a Vita budget;
 * tune them if the game reports an allocator running dry. */
#define MHS_ALLOC_TABLE_OFF   0x154ACEC
#define MHS_NOTIFYONCREATE_OFF 0x87d501   /* Thumb address of notifyOnCreate */

static const struct { const char *name; uint32_t orig, vita; } mhs_heaps[9] = {
    { "Global",       0x00800000, 8   << 20 },
    { "Temp",         0x08000000, 16  << 20 },
    { "Resource",     0x0d85c000, 80  << 20 },
    { "Unit/Area",    0x01400000, 12  << 20 },  /* skipped by the game's loop */
    { "System",       0x06400000, 24  << 20 },
    { "Array/String", 0x04000000, 16  << 20 },
    { "Collision",    0x00400000, 4   << 20 },
    { "Develop",      0x04000000, 2   << 20 },  /* debug tooling only */
    { "Sound",        0x08000000, 16  << 20 },
};

static void patch_heap_sizes(void) {
    uintptr_t base = so_symbol(&so_mod, "notifyOnCreate") - MHS_NOTIFYONCREATE_OFF;
    uint32_t *table = (uint32_t *)(base + MHS_ALLOC_TABLE_OFF);

    /* Only touch it if every entry still holds the stock size, so a different
     * game build can't get corrupted by a stale offset. */
    for (int i = 0; i < 9; i++) {
        if (table[i * 6 + 3] != mhs_heaps[i].orig) {
            l_debug("patch: heap table mismatch at %d (%s): %#x, leaving sizes alone",
                    i, mhs_heaps[i].name, table[i * 6 + 3]);
            return;
        }
    }
    for (int i = 0; i < 9; i++) {
        uint32_t size = mhs_heaps[i].vita;
        kuKernelCpuUnrestrictedMemcpy(&table[i * 6 + 3], &size, sizeof(size));
        l_debug("patch: heap %-12s %3u MB -> %3u MB", mhs_heaps[i].name,
                mhs_heaps[i].orig >> 20, size >> 20);
    }
}

/* MtDebug's handler setters are members of MtDebug::mInstance:
 * set*Handler(this, void (*fn)(const char *msg, void *ctx), void *ctx).
 * Called from main() after the game's static constructors have run, since
 * those install the (empty) default handlers. */
void mhs_install_mt_handlers(void) {
    typedef void (*SetHandler)(void *self, void (*fn)(const char *, void *), void *ctx);
    static const char *setters[] = {
        "_ZN7MtDebug15setTraceHandlerEPFvPKcPvES2_",
        "_ZN7MtDebug15setErrorHandlerEPFvPKcPvES2_",
        "_ZN7MtDebug17setWarningHandlerEPFvPKcPvES2_",
        "_ZN7MtDebug14setInfoHandlerEPFvPKcPvES2_",
    };
    void *inst = (void *)so_symbol(&so_mod, "_ZN7MtDebug9mInstanceE");
    if (!inst) return;
    for (unsigned i = 0; i < sizeof(setters) / sizeof(*setters); i++) {
        SetHandler set = (SetHandler)so_symbol(&so_mod, setters[i]);
        if (set) set(inst, mhs_hook_handler, NULL);
    }
    /* Turn the engine's own message output on. */
    uint8_t one = 1;
    uintptr_t en = so_symbol(&so_mod, "_ZN7MtDebug8mEnabledE");
    uintptr_t ew = so_symbol(&so_mod, "_ZN7MtDebug17mEnabledErrorWarnE");
    if (en) kuKernelCpuUnrestrictedMemcpy((void *)en, &one, 1);
    if (ew) kuKernelCpuUnrestrictedMemcpy((void *)ew, &one, 1);
}

/* native::multithread::Event::wait(unsigned ms) waits on its condvar once and
 * then treats the event as signalled, without re-checking the flag. Through the
 * boilerplate's bionic pthread emulation that wait can return early, which let
 * File::readAsync clear a request while the FileAccessManager thread was still
 * reading it ("readAsyncDirect: Critical error ... error=-1" on the OBB).
 * Replacement with the same layout and return value, looping on the flag. */
typedef struct {
    volatile uint8_t signaled;   /* +0 */
    uint8_t pad[3];
    int mutex;                   /* +4  bionic pthread_mutex_t */
    int cond;                    /* +8  bionic pthread_cond_t  */
} NativeEvent;

extern int pthread_mutex_lock_soloader(void *m);
extern int pthread_mutex_unlock_soloader(void *m);
extern int pthread_cond_wait_soloader(void *c, void *m);
extern int pthread_cond_timedwait_soloader(void *c, void *m, struct timespec *abstime);

#define EVENT_INFINITE 0x80000000u

static uint64_t now_us(void) { return sceKernelGetProcessTimeWide(); }

static int mhs_event_wait(NativeEvent *ev, unsigned timeout_ms) {
    pthread_mutex_lock_soloader(&ev->mutex);
    int ok = 1;
    if (timeout_ms == EVENT_INFINITE) {
        while (!ev->signaled) {
            if (pthread_cond_wait_soloader(&ev->cond, &ev->mutex) != 0) {
                pthread_mutex_unlock_soloader(&ev->mutex);
                sceKernelDelayThread(1000);
                pthread_mutex_lock_soloader(&ev->mutex);
            }
        }
    } else {
        uint64_t deadline = now_us() + (uint64_t)timeout_ms * 1000;
        while (!ev->signaled) {
            uint64_t t = now_us();
            if (t >= deadline) { ok = 0; break; }
            struct timeval tv;
            gettimeofday(&tv, NULL);
            uint64_t abs_us = (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec + (deadline - t);
            struct timespec ts = { (time_t)(abs_us / 1000000), (long)(abs_us % 1000000) * 1000 };
            if (pthread_cond_timedwait_soloader(&ev->cond, &ev->mutex, &ts) != 0 && !ev->signaled) {
                pthread_mutex_unlock_soloader(&ev->mutex);
                sceKernelDelayThread(500);
                pthread_mutex_lock_soloader(&ev->mutex);
            }
        }
    }
    if (ok) ev->signaled = 0;
    pthread_mutex_unlock_soloader(&ev->mutex);
    return ok;
}

static void ret_void(void) {}

void so_patch(void) {
    kuser_patch();
    mhs_libmhr_base = so_symbol(&so_mod, "notifyOnCreate") - MHS_NOTIFYONCREATE_OFF;
    patch_heap_sizes();

    /* Fatal paths: show the game's own message instead of exiting silently.
     * (native::debug::trace and MtDebug's default handlers are 2-byte stubs
     * packed back to back, too small to hook safely; mhs_install_mt_handlers
     * covers the MtDebug ones instead.) */
    hook_sym("_ZN7MtDebug5abortEPKcz", (uintptr_t)&mhs_hook_mtdebug_abort);
    hook_sym("_ZN6native5debug12abortProgramEPKcz", (uintptr_t)&mhs_hook_abort_program);

    hook_sym("_ZN6native11multithread5Event4waitEj", (uintptr_t)&mhs_event_wait);

    /* Candidate (untested): the Android sound backend builds an OpenSL reverb
     * chain the Vita wrapper doesn't implement. If audio init fails, try
     * skipping it. Check createEffect's return type in Ghidra first: the
     * mangled name doesn't encode it, and callers may use the result.
     *
     * hook_sym("_ZN6sSound19NativeSystemAndroid12createEffectEjj", (uintptr_t)&ret_void);
     * hook_sym("_ZN6sSound19NativeSystemAndroid12updateEffectEv", (uintptr_t)&ret_void);
     */
    (void)ret_void;

    /* TODO (needs testing on hardware):
     *  - Movie playback: hook the uMovie / MTFPMoviePlayer wrapper to end
     *    immediately if the JNI stubs in java.c aren't enough.
     *  - Performance: lower internal render resolution / shadow quality via
     *    the engine's sMain / sRender properties once we know which presets
     *    the game exposes.
     */
}
