/*
 * Monster Hunter Stories (Android 1.0.5) -> PS Vita
 *
 * Replaces soloader-boilerplate's source/main.c.
 *
 * Instead of JNI_OnLoad + a Java activity, we drive libMHR.so's exported
 * MTFPJNI entry points directly in the same order Android would:
 *   onCreate -> onStart -> onResume -> (GL surface) onSurfaceChanged ->
 *   renderFrame every frame, with input delivered as MTFPEvents.
 */

#include "utils/init.h"
#include "utils/glutil.h"
#include "utils/logger.h"
#include "utils/dialog.h"

#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/power.h>
#include <math.h>
#include <string.h>

#include <falso_jni/FalsoJNI.h>
#include <so_util/so_util.h>
#include <vitaGL.h>
#include <vitashark.h>
#include <stdlib.h>
#include <stdio.h>

#include "reimpl/controls.h"
#include "mhs/mhs_native.h"
#include "mhs/mtfp_event.h"
#include "mhs/dynlib_extra.h"

/* newlib heap: holds the game's MT Framework heaps (~166 MB, patch.c) plus its
 * direct mallocs. vitaGL is initialised afterwards and gets what's left of main
 * memory for its pools; at 300 MB here it ran dry and crashed in glLinkProgram. */
int _newlib_heap_size_user = 230 * 1024 * 1024;

/* The game runs on this thread and links its shaders deep inside the engine's
 * call chain, where SceShaccCg (vitaGL's runtime shader compiler) then needs a
 * lot of stack. With the default ~256 KB it failed with "fatal internal error"
 * even on trivial shaders that compiled fine from main(). */
unsigned int sceUserMainThreadStackSize = 4 * 1024 * 1024;

#ifdef USE_SCELIBC_IO
int sceLibcHeapSize = 4 * 1024 * 1024;
#endif

so_module so_mod;
MhsNative mhs;

#define SCREEN_W 960
#define SCREEN_H 544

/* ------------------------------------------------------------------------ */
/* Entry points                                                             */
/* ------------------------------------------------------------------------ */

#define RESOLVE(name)                                                        \
    do {                                                                     \
        *(uintptr_t *)&mhs.name = so_symbol(&so_mod, #name);                 \
        if (!mhs.name) fatal_error("libMHR.so is missing export " #name);   \
    } while (0)

void mhs_native_resolve(void) {
    RESOLVE(notifyOnCreate);
    RESOLVE(notifyOnStart);
    RESOLVE(notifyOnResume);
    RESOLVE(notifyOnPause);
    RESOLVE(notifyOnStop);
    RESOLVE(notifyOnDestroy);
    RESOLVE(notifyOnSurfaceChanged);
    RESOLVE(renderFrame);
    RESOLVE(notifyEvent);
}

/* ------------------------------------------------------------------------ */
/* Controls                                                                 */
/* ------------------------------------------------------------------------ */

/* The game has native gamepad support (sAppPad, key assignment UI, Shield /
 * Nyko / generic Android pad profiles), so physical buttons are forwarded as
 * standard Android gamepad key codes. reimpl/controls.c maps Vita buttons to
 * these codes; see README for the layout. */
void controls_handler_key(int32_t keycode, ControlsAction action) {
    if (action == CONTROLS_ACTION_MOVE) return;
    mtfp_send_gamepad_key(keycode, action == CONTROLS_ACTION_DOWN);
}

/* Sticks arrive one at a time from controls.c; the game wants a single motion
 * event carrying every axis, so collect them and flush once per frame. */
static float stick_lx, stick_ly, stick_rx, stick_ry;
static float sent_lx, sent_ly, sent_rx, sent_ry;
static bool sticks_dirty;

void controls_handler_analog(ControlsStickId which, float x, float y, ControlsAction action) {
    if (which == CONTROLS_STICK_LEFT) { stick_lx = x; stick_ly = y; }
    else                              { stick_rx = x; stick_ry = y; }
    sticks_dirty = true;
}

static void flush_sticks(void) {
    if (!sticks_dirty) return;
    sticks_dirty = false;
    if (stick_lx == sent_lx && stick_ly == sent_ly && stick_rx == sent_rx && stick_ry == sent_ry)
        return;
    sent_lx = stick_lx; sent_ly = stick_ly; sent_rx = stick_rx; sent_ry = stick_ry;
    mtfp_send_gamepad_axes(stick_lx, stick_ly, stick_rx, stick_ry, 0, 0, 0, 0);
}

/* Touch: reproduce what Android's GestureDetector + MTFPActivity.onTouchEvent
 * would report, so the touch UI keeps working alongside the buttons. */
#define MAX_FINGERS        6
#define TAP_MAX_US         300000
#define LONG_PRESS_US      500000
#define TAP_SLOP_PX        12.0f

typedef struct {
    bool active;
    int32_t id;
    float start_x, start_y;
    uint64_t start_us;
    bool moved, long_pressed;
} Finger;

static Finger fingers[MAX_FINGERS];

static int finger_count(void) {
    int n = 0;
    for (int i = 0; i < MAX_FINGERS; i++) n += fingers[i].active;
    return n;
}

static Finger *finger_find(int32_t id, bool create) {
    for (int i = 0; i < MAX_FINGERS; i++)
        if (fingers[i].active && fingers[i].id == id) return &fingers[i];
    if (!create) return NULL;
    for (int i = 0; i < MAX_FINGERS; i++)
        if (!fingers[i].active) return &fingers[i];
    return NULL;
}

void controls_handler_touch(int32_t id, float x, float y, ControlsAction action) {
    uint64_t now = sceKernelGetProcessTimeWide();
    Finger *f = finger_find(id, action == CONTROLS_ACTION_DOWN);
    if (!f) return;
    int index = (int)(f - fingers);

    switch (action) {
    case CONTROLS_ACTION_DOWN:
        *f = (Finger){ .active = true, .id = id, .start_x = x, .start_y = y, .start_us = now };
        mtfp_send_gesture(GestureOnDown, x, y, id, finger_count(), index);
        break;

    case CONTROLS_ACTION_MOVE:
        if (fabsf(x - f->start_x) > TAP_SLOP_PX || fabsf(y - f->start_y) > TAP_SLOP_PX)
            f->moved = true;
        if (!f->moved && !f->long_pressed && now - f->start_us >= LONG_PRESS_US) {
            f->long_pressed = true;
            mtfp_send_gesture(GestureOnLongPress, x, y, id, 1, 0);
        }
        mtfp_send_gesture(GestureOnScroll, x, y, id, finger_count(), index);
        break;

    case CONTROLS_ACTION_UP:
        if (!f->moved && !f->long_pressed && now - f->start_us <= TAP_MAX_US) {
            mtfp_send_gesture(GestureOnSingleTapUp, x, y, id, finger_count(), index);
            mtfp_send_gesture(GestureOnSingleTapConfirmed, x, y, id, 1, 0);
        }
        mtfp_send_gesture(GestureOnTouchRelease, x, y, id, finger_count(), index);
        f->active = false;
        break;
    }
}

/* ------------------------------------------------------------------------ */
/* Main                                                                     */
/* ------------------------------------------------------------------------ */

int main(void) {
    /* Max clocks: this is a heavy 3D game for the Vita. */
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

    soloader_init_all();      /* loads SO_PATH (libMHR.so), resolves imports, runs so_patch() */
    mhs_native_resolve();
    mhs_java_init_fields();
    mhs_install_mt_handlers();
    mhs_log("MHS: loader up, libMHR.so at %p", (void *)mhs_libmhr_base);

    gl_init();                /* vitaGL context; the game issues GLES2 calls from renderFrame */
    mhs_log("MHS: vitaGL free memory: VRAM %u KB, RAM %u KB, PHYCONT %u KB",
            (unsigned)(vglMemFree(VGL_MEM_VRAM) >> 10), (unsigned)(vglMemFree(VGL_MEM_RAM) >> 10),
            (unsigned)(vglMemFree(VGL_MEM_SLOW) >> 10));
    /* vitaGL hands SceShaccCg its own pool (vglMalloc), which the game fills
     * with textures before its first shader link. Use the newlib heap for
     * compiler scratch. Builds up to 27 called shark_set_allocators() here,
     * which only takes effect in shark_init(), so the module has to be
     * reloaded for the allocator to change. */
    mhs_shader_compiler_reload();
    mhs_shader_selftest();
    /* Build 28: threads alone (T0/T1) were fine; the compiler broke in
     * filesystem::initialize (512 Events, see patch.c). Kept as a check. */
    mhs_thread_probe();
    controls_init();

    l_debug("MHS: notifyOnCreate (stepwise)");
    /* Same steps as libMHR's notifyOnCreate, run one at a time with a shader
     * compiler self-test after each, to find the one that breaks SceShaccCg. */
    {
        void (*setJavaVM)(void *) = (void *)so_symbol(&so_mod, "_ZN6native7android9setJavaVMEP7_JavaVM");
        void (*native_init)(void *) = (void *)so_symbol(&so_mod, "_ZN6native10initializeEP8_jobject");
        void (*setAssetMgr)(void *, void *) = (void *)so_symbol(&so_mod, "_ZN6native10filesystem15setAssetManagerEP7_JNIEnvP8_jobject");
        void (*startGame)(void) = (void *)so_symbol(&so_mod, "_ZN6native7android15startGameThreadEv");
        if (setJavaVM && native_init && setAssetMgr && startGame) {
            setJavaVM(&jvm);
            mhs_selftest_point("S1 after setJavaVM");
            /* native::initialize is just these calls in this order (build 27:
             * the compiler broke somewhere inside it). */
            static const char *const subinit[] = {
                "_ZN6native7android10initializeEP8_jobject",
                "_ZN6native11performance10initializeEv",
                "_ZN6native5debug10initializeEv",
                "_ZN6native6memory10initializeEv",
                "_ZN6native11multithread10initializeEv",
                "_ZN6native10filesystem10initializeEv",
                "_ZN6native10filesystem12initRootPathEv",
                "_ZN6native9clipboard10initializeEv",
                "_ZN6native10multimedia10initializeEv",
                "_ZN6native7console10initializeEv",
                "_ZN6native10googleplay10initializeEv",
                "_ZN6native10httpclient10initializeEv",
            };
            int all_found = 1;
            for (unsigned i = 0; i < sizeof(subinit) / sizeof(*subinit); i++)
                if (!so_symbol(&so_mod, subinit[i])) { mhs_log("MHS: missing %s", subinit[i]); all_found = 0; }
            if (all_found) {
                for (unsigned i = 0; i < sizeof(subinit) / sizeof(*subinit); i++) {
                    void (*fn)(void *) = (void *)so_symbol(&so_mod, subinit[i]);
                    char label[96];
                    fn(MHS_FAKE_ACTIVITY);   /* only android::initialize takes an argument */
                    snprintf(label, sizeof(label), "S2.%u after %s", i, subinit[i]);
                    mhs_selftest_point(label);
                }
            } else {
                native_init(MHS_FAKE_ACTIVITY);
                mhs_selftest_point("S2 after native::initialize");
            }
            setAssetMgr(&jni, MHS_FAKE_ASSET_MANAGER);
            mhs_selftest_point("S3 after setAssetManager");
            startGame();
            mhs_selftest_point("S4 after startGameThread");
        } else {
            mhs_log("MHS: stepwise symbols missing, using notifyOnCreate");
            mhs.notifyOnCreate(&jni, MHS_FAKE_CLASS, MHS_FAKE_ACTIVITY, &jvm, MHS_FAKE_ASSET_MANAGER);
        }
    }
    mhs_selftest_point("P1 plain, after notifyOnCreate");
    mhs.notifyOnStart(&jni, MHS_FAKE_CLASS);
    mhs.notifyOnResume(&jni, MHS_FAKE_CLASS);
    mhs_selftest_point("P2 plain, after notifyOnResume");

    l_debug("MHS: notifyOnSurfaceChanged %dx%d", SCREEN_W, SCREEN_H);
    mhs.notifyOnSurfaceChanged(&jni, MHS_FAKE_CLASS, SCREEN_W, SCREEN_H);
    mhs_selftest_point("P3 plain, after notifyOnSurfaceChanged");

    while (1) {
        controls_poll();
        flush_sticks();
        mhs.renderFrame(&jni, MHS_FAKE_CLASS);
        gl_swap();
    }

    sceKernelExitDeleteThread(0);
    return 0;
}
