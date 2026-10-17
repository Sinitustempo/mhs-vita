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
#include <vitasdk.h>

extern so_module so_mod;

#define SCE_KERNEL_MEMBLOCK_TYPE_USER_RX (0x0C20D050)

#include "utils/logger.h"
#include "utils/dialog.h"
#include "reimpl/sys.h"

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

static void ret_void(void) {}

void so_patch(void) {
    kuser_patch();

    /* Candidate (untested): the Android sound backend builds an OpenSL reverb
     * chain the Vita wrapper doesn't implement. If audio init fails, try
     * skipping it. Check createEffect's return type in Ghidra first: the
     * mangled name doesn't encode it, and callers may use the result.
     *
     * hook_sym("_ZN6sSound19NativeSystemAndroid12createEffectEjj", (uintptr_t)&ret_void);
     * hook_sym("_ZN6sSound19NativeSystemAndroid12updateEffectEv", (uintptr_t)&ret_void);
     */
    (void)ret_void;
    (void)hook_sym;

    /* TODO (needs testing on hardware):
     *  - Movie playback: hook the uMovie / MTFPMoviePlayer wrapper to end
     *    immediately if the JNI stubs in java.c aren't enough.
     *  - Performance: lower internal render resolution / shadow quality via
     *    the engine's sMain / sRender properties once we know which presets
     *    the game exposes.
     */
}
