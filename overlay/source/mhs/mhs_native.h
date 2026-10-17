/*
 * Monster Hunter Stories (Android 1.0.5) -> PS Vita
 *
 * Entry points exported by libMHR.so (plain C symbols). On Android the Java
 * class MTFPJNI declares these as native; libAndroidSkeletonBoot.so registers
 * them and forwards each call to the same-named export in libMHR.so. We skip
 * the boot library and call libMHR.so directly.
 *
 * Java declarations (from classes.dex):
 *   static native void    notifyOnCreate(Activity, String nativeLibPath, AssetManager);
 *   static native void    notifyOnSurfaceChanged(int width, int height);
 *   static native void    renderFrame();
 *   static native boolean notifyEvent(MTFPEvent);
 *   static native void    notifyOnStart/Resume/Pause/Stop/Restart/Destroy();
 *   static native void    notifyOnGLContextLost/SurfaceDestroyStart/SurfaceDestroyed();
 *
 * TODO(verify in Ghidra): the boot wrappers are assumed to pass (env, clazz,
 * args...) through unchanged. Check Java_jp_co_capcom_mhsspen_MTFPJNI_notifyOnCreate
 * in libAndroidSkeletonBoot.so before trusting this.
 */

#ifndef MHS_NATIVE_H
#define MHS_NATIVE_H

#include <falso_jni/FalsoJNI.h>

typedef struct {
    void     (*notifyOnCreate)(JNIEnv *, jclass, jobject activity, jstring lib_dir, jobject asset_mgr);
    void     (*notifyOnStart)(JNIEnv *, jclass);
    void     (*notifyOnResume)(JNIEnv *, jclass);
    void     (*notifyOnPause)(JNIEnv *, jclass);
    void     (*notifyOnStop)(JNIEnv *, jclass);
    void     (*notifyOnDestroy)(JNIEnv *, jclass);
    void     (*notifyOnSurfaceChanged)(JNIEnv *, jclass, jint width, jint height);
    void     (*renderFrame)(JNIEnv *, jclass);
    jboolean (*notifyEvent)(JNIEnv *, jclass, jobject event);
} MhsNative;

extern MhsNative mhs;

/* Resolves every pointer in `mhs` from so_mod; aborts with a dialog on failure. */
void mhs_native_resolve(void);

/* Opaque non-NULL handles we hand to the game as Java objects. */
#define MHS_FAKE_ACTIVITY      ((jobject)0x41435449) /* "ACTI" */
#define MHS_FAKE_ASSET_MANAGER ((jobject)0x41534D47) /* "ASMG" */
#define MHS_FAKE_CLASS         ((jclass)0x434C5353)  /* "CLSS" */

#endif
