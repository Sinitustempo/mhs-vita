/*
 * Monster Hunter Stories (Android 1.0.5) -> PS Vita
 *
 * Replaces soloader-boilerplate's source/java.c.
 *
 * FalsoJNI answers the game's Java calls by method *name*, so every Java
 * method libMHR.so looks up must be listed here. The list was built from the
 * GetMethodID/GetStaticMethodID strings in libMHR.so plus the matching
 * declarations in classes.dex. Anything missing shows up at runtime as a
 * "[GetMethodID] unknown method" line in the FalsoJNI log: add it here.
 *
 * Paths: the game builds its OBB path as "<external storage>/Android/obb/<pkg>",
 * so with the root below it reads
 *   ux0:data/mhs/Android/obb/jp.co.capcom.mhsspen/main.5.jp.co.capcom.mhsspen.obb
 */

#include <falso_jni/FalsoJNI.h>
#include <falso_jni/FalsoJNI_Impl.h>
#include <falso_jni/FalsoJNI_Logger.h>

#include <string.h>

#include "mhs/mtfp_event.h"

#define MHS_STORAGE_ROOT "ux0:data/mhs"
#define MHS_PACKAGE      "jp.co.capcom.mhsspen"

/* Device id we report for the Vita's built-in controls (see mtfp_event.c). */
#define VITA_PAD_DEVICE_ID 1

static jstring str(const char *s) { return jni->NewStringUTF(&jni, s); }

/* ------------------------------------------------------------------------ */
/* MTFPEvent: reads come from mtfp_current_event                            */
/* ------------------------------------------------------------------------ */

static MtfpParam *ev_param(va_list args) {
    int idx = va_arg(args, int);
    if (!mtfp_current_event || idx < 0 || idx >= mtfp_current_event->num) return NULL;
    return &mtfp_current_event->params[idx];
}

jobject getEventName(jmethodID id, va_list args) {
    return mtfp_current_event ? str(mtfp_current_event->name) : NULL;
}
jint getParameterNum(jmethodID id, va_list args) {
    return mtfp_current_event ? mtfp_current_event->num : 0;
}
jint getParameterType(jmethodID id, va_list args) {
    MtfpParam *p = ev_param(args);
    return p ? p->type : MTFP_PARAM_INVALID;
}
jboolean getBooleanParameter(jmethodID id, va_list args) { MtfpParam *p = ev_param(args); return p ? p->b : JNI_FALSE; }
jint     getIntegerParameter(jmethodID id, va_list args) { MtfpParam *p = ev_param(args); return p ? p->i : 0; }
jfloat   getFloatParameter(jmethodID id, va_list args)   { MtfpParam *p = ev_param(args); return p ? p->f : 0.0f; }
jlong    getLongParameter(jmethodID id, va_list args)    { MtfpParam *p = ev_param(args); return p ? p->l : 0; }
jobject  getStringParameter(jmethodID id, va_list args)  { MtfpParam *p = ev_param(args); return (p && p->s) ? str(p->s) : NULL; }
jobject  getObjectParameter(jmethodID id, va_list args)  { MtfpParam *p = ev_param(args); return p ? p->o : NULL; }

/* ------------------------------------------------------------------------ */
/* MTFPActivity / Context                                                   */
/* ------------------------------------------------------------------------ */

jobject getLanguage(jmethodID id, va_list args)    { return str("en"); }
jobject getVersionName(jmethodID id, va_list args) { return str("1.0.5"); }
jobject getPackageName(jmethodID id, va_list args) { return str(MHS_PACKAGE); }
jint getVersionCode(jmethodID id, va_list args)    { return 5; }
jint getMainOBBVersion(jmethodID id, va_list args)  { return 5; }
jint getPatchOBBVersion(jmethodID id, va_list args) { return 5; }

/* TODO(verify): value the game treats as "license OK". 0 is a guess. */
jint getLVLState(jmethodID id, va_list args) { return 0; }

jboolean getKeepScreenOn(jmethodID id, va_list args) { return JNI_TRUE; }
jint getScreenOrientation(jmethodID id, va_list args) { return 2; /* DeviceLandscapeLeft */ }
jint getDeviceRequestedOrientation(jmethodID id, va_list args) { return 2; }
jboolean moveTaskToBack(jmethodID id, va_list args) { return JNI_FALSE; }

/* System font rendering (Java draws glyphs into bitmaps). Report none so the
 * game falls back to its bundled fonts. TODO(verify) the game copes with 0. */
jint getSystemFontDataNum(jmethodID id, va_list args) { return 0; }
jobject getByteArray(jmethodID id, va_list args) { return NULL; }

jobject getFileName(jmethodID id, va_list args) {
    jstring name = va_arg(args, jstring);
    return name; /* passthrough */
}
jobject getSharedFileDir(jmethodID id, va_list args) { return str(MHS_STORAGE_ROOT "/files"); }

/* java.io.File as returned by getExternalStorageDirectory()/getExternalFilesDir().
 * FalsoJNI can't tell File objects apart, so both resolve to the same root. */
jobject getExternalStorageDirectory(jmethodID id, va_list args) { return str(MHS_STORAGE_ROOT); }
jobject getExternalFilesDir(jmethodID id, va_list args)        { return str(MHS_STORAGE_ROOT); }
jobject getAbsolutePath(jmethodID id, va_list args)            { return str(MHS_STORAGE_ROOT); }
jobject getPath(jmethodID id, va_list args)                    { return str(MHS_STORAGE_ROOT); }
jboolean isExternalStorageMounted(jmethodID id, va_list args)   { return JNI_TRUE; }
jboolean isExternalStorageRemovable(jmethodID id, va_list args) { return JNI_FALSE; }
jboolean createDir(jmethodID id, va_list args) { return JNI_TRUE; }
jboolean createDirRecursive(jmethodID id, va_list args) { return JNI_TRUE; }

/* Performance hints the engine queries at boot. Reporting a mid-range phone
 * nudges MT Framework toward its lighter quality presets. */
jobject getRuntime(jmethodID id, va_list args) { return (jobject)0x52554E54; /* "RUNT" */ }
jint availableProcessors(jmethodID id, va_list args) { return 3; /* cores usable by apps */ }
jlong getAvailMemory(jmethodID id, va_list args) { return 256LL * 1024 * 1024; }

/* ------------------------------------------------------------------------ */
/* Gamepad (MTFPUtils)                                                      */
/* ------------------------------------------------------------------------ */

void enumGamePadDevice(jmethodID id, va_list args) {}

jboolean isInputDeviceAvailable(jmethodID id, va_list args) {
    return va_arg(args, int) == VITA_PAD_DEVICE_ID ? JNI_TRUE : JNI_FALSE;
}

/* sPad::Map picks a button profile by device name ("NVIDIA", "Shield",
 * "Nyko", else generic). The Shield profile matches the Vita's face-button
 * layout, so claim to be a Shield controller. */
jobject getInputDeviceName(jmethodID id, va_list args) {
    return va_arg(args, int) == VITA_PAD_DEVICE_ID ? str("NVIDIA Controller v01.03") : NULL;
}

/* ------------------------------------------------------------------------ */
/* Movies (MTFPMoviePlayer): not implemented yet                            */
/* ------------------------------------------------------------------------ */
/* Android plays the FMVs (assets/nativeAndroid/movie/p0x0.mp4) with
 * MediaPlayer into a SurfaceTexture the game samples as GL_OES_EGL_image_external.
 * TODO: either decode with the Vita's SceAvPlayer into a vitaGL texture, or
 * make the player report "finished" immediately so cutscenes are skipped. The
 * zero duration below is a first attempt at the latter. */
void mp_play(jmethodID id, va_list args) {}
void mp_stop(jmethodID id, va_list args) {}
void mp_pause(jmethodID id, va_list args) {}
void mp_prepare(jmethodID id, va_list args) {}
void mp_seek(jmethodID id, va_list args) {}
void mp_cleanUp(jmethodID id, va_list args) {}
void mp_setAssetPath(jmethodID id, va_list args) {}
void mp_setExternalStoragePath(jmethodID id, va_list args) {}
void mp_setSurface(jmethodID id, va_list args) {}
void mp_setVolume(jmethodID id, va_list args) {}
void mp_setSubtitle(jmethodID id, va_list args) {}
void mp_updateTexImage(jmethodID id, va_list args) {}
void mp_getTransformMatrix(jmethodID id, va_list args) {}
jint mp_getDuration(jmethodID id, va_list args) { return 0; }
jint mp_getCurrentPosition(jmethodID id, va_list args) { return 0; }
jint mp_getVideoWidth(jmethodID id, va_list args) { return 0; }
jint mp_getVideoHeight(jmethodID id, va_list args) { return 0; }

/* ------------------------------------------------------------------------ */
/* Google Play Games / cloud saves / Bluetooth / QR: report "unavailable"   */
/* ------------------------------------------------------------------------ */

void noop(jmethodID id, va_list args) {}
jboolean ret_false(jmethodID id, va_list args) { return JNI_FALSE; }
jboolean ret_true(jmethodID id, va_list args) { return JNI_TRUE; }
jint ret_zero(jmethodID id, va_list args) { return 0; }
jint ret_minus1(jmethodID id, va_list args) { return -1; }
jobject ret_null(jmethodID id, va_list args) { return NULL; }
jobject ret_empty_str(jmethodID id, va_list args) { return str(""); }

/* ------------------------------------------------------------------------ */
/* Tables                                                                   */
/* ------------------------------------------------------------------------ */

enum {
    /* MTFPEvent */
    M_getEventName = 100, M_getParameterNum, M_getParameterType, M_getBooleanParameter,
    M_getIntegerParameter, M_getFloatParameter, M_getLongParameter, M_getStringParameter,
    M_getObjectParameter,
    /* Activity / system */
    M_getLanguage = 200, M_getVersionName, M_getPackageName, M_getVersionCode,
    M_getMainOBBVersion, M_getPatchOBBVersion, M_getLVLState, M_getKeepScreenOn,
    M_getScreenOrientation, M_getDeviceRequestedOrientation, M_moveTaskToBack,
    M_getSystemFontDataNum, M_getByteArray, M_getFileName, M_getSharedFileDir,
    M_getExternalStorageDirectory, M_getExternalFilesDir, M_getAbsolutePath, M_getPath,
    M_isExternalStorageMounted, M_isExternalStorageRemovable, M_createDir,
    M_createDirRecursive, M_getRuntime, M_availableProcessors, M_getAvailMemory,
    M_getClipboard, M_screencapture, M_getEditText,
    /* void no-ops */
    M_removeSystemFontDataAll = 300, M_resumeCheckLicense, M_setClipboard,
    M_hideNavigationBar, M_setKeepScreenOn, M_openURL, M_openTwitter, M_finish,
    M_hideKeyInput, M_addView, M_removeView, M_setViewVisibility, M_setTag,
    M_setBackgroundColor, M_setString, M_getTextBounds, M_sendNotification,
    M_deleteNotification, M_openImagePicker, M_openQRCodeDecoder, M_closeQRCodeDecoder,
    M_openQRCodeEncoder, M_closeQRCodeEncoder, M_showInfoDialog, M_showSelectDialog,
    M_showProgressDialogBinding, M_dissmissProgressDialog, M_getAndroidVolume,
    M_getAudioTrackBufferSize, M_kill,
    /* gamepad */
    M_enumGamePadDevice = 400, M_isInputDeviceAvailable, M_getInputDeviceName,
    /* movie */
    M_play = 500, M_stop, M_pause, M_prepare, M_seek, M_cleanUp, M_setAssetPath,
    M_setExternalStoragePath, M_setSurface, M_setVolume, M_setSubtitle,
    M_updateTexImage, M_getTransformMatrix, M_getDuration, M_getCurrentPosition,
    M_getVideoWidth, M_getVideoHeight,
    /* online services: all disabled */
    M_isSignedIn = 600, M_isConnecting, M_isSignedFailed, M_hasSignInError,
    M_isOnline, M_isConnected, M_isSuccess, M_isResolveConflictFailed,
    M_readSavedGames, M_writeSavedGames, M_resolveConflict, M_init, M_start,
    M_isOverLen,
    M_isSignedCancelled = 650,
    M_beginUserInitiatedSignIn = 660, M_signOut, M_reconnectClient,
    M_setMaxAutoSignInAttempts, M_showSavedGamesUI, M_setUseServerVersion,
    M_showAchievement, M_createPlayersClient, M_createRoom, M_sendData,
    M_setLocalSessionId, M_releaseSession, M_listen_close, M_clearConnectWork,
    M_setSessionLock, M_ensureBluetoothDiscoverable, M_discoverBluetoothDevice,
    M_onStop,
    M_getStatusCode = 700, M_loadAchievement, M_leaveRoom, M_getStartingMemberListNum,
    M_getDeviceNum, M_getDeviceInfo, M_listen, M_connect, M_sendSessionData,
    M_getMaxDataSize,
    M_getApiClient = 750, M_getSelfPlayerId, M_getStartingMemberPlayerId,
    M_getBluetoothLocalAddress, M_getConnectAddress,
};

NameToMethodID nameToMethodId[] = {
    { M_getEventName,        "getEventName",        METHOD_TYPE_OBJECT },
    { M_getParameterNum,     "getParameterNum",     METHOD_TYPE_INT },
    { M_getParameterType,    "getParameterType",    METHOD_TYPE_INT },
    { M_getBooleanParameter, "getBooleanParameter", METHOD_TYPE_BOOLEAN },
    { M_getIntegerParameter, "getIntegerParameter", METHOD_TYPE_INT },
    { M_getFloatParameter,   "getFloatParameter",   METHOD_TYPE_FLOAT },
    { M_getLongParameter,    "getLongParameter",    METHOD_TYPE_LONG },
    { M_getStringParameter,  "getStringParameter",  METHOD_TYPE_OBJECT },
    { M_getObjectParameter,  "getObjectParameter",  METHOD_TYPE_OBJECT },

    { M_getLanguage,                  "getLanguage",                  METHOD_TYPE_OBJECT },
    { M_getVersionName,               "getVersionName",               METHOD_TYPE_OBJECT },
    { M_getPackageName,               "getPackageName",               METHOD_TYPE_OBJECT },
    { M_getVersionCode,               "getVersionCode",               METHOD_TYPE_INT },
    { M_getMainOBBVersion,            "getMainOBBVersion",            METHOD_TYPE_INT },
    { M_getPatchOBBVersion,           "getPatchOBBVersion",           METHOD_TYPE_INT },
    { M_getLVLState,                  "getLVLState",                  METHOD_TYPE_INT },
    { M_getKeepScreenOn,              "getKeepScreenOn",              METHOD_TYPE_BOOLEAN },
    { M_getScreenOrientation,         "getScreenOrientation",         METHOD_TYPE_INT },
    { M_getDeviceRequestedOrientation,"getDeviceRequestedOrientation",METHOD_TYPE_INT },
    { M_moveTaskToBack,               "moveTaskToBack",               METHOD_TYPE_BOOLEAN },
    { M_getSystemFontDataNum,         "getSystemFontDataNum",         METHOD_TYPE_INT },
    { M_getByteArray,                 "getByteArray",                 METHOD_TYPE_OBJECT },
    { M_getFileName,                  "getFileName",                  METHOD_TYPE_OBJECT },
    { M_getSharedFileDir,             "getSharedFileDir",             METHOD_TYPE_OBJECT },
    { M_getExternalStorageDirectory,  "getExternalStorageDirectory",  METHOD_TYPE_OBJECT },
    { M_getExternalFilesDir,          "getExternalFilesDir",          METHOD_TYPE_OBJECT },
    { M_getAbsolutePath,              "getAbsolutePath",              METHOD_TYPE_OBJECT },
    { M_getPath,                      "getPath",                      METHOD_TYPE_OBJECT },
    { M_isExternalStorageMounted,     "isExternalStorageMounted",     METHOD_TYPE_BOOLEAN },
    { M_isExternalStorageRemovable,   "isExternalStorageRemovable",   METHOD_TYPE_BOOLEAN },
    { M_createDir,                    "createDir",                    METHOD_TYPE_BOOLEAN },
    { M_createDirRecursive,           "createDirRecursive",           METHOD_TYPE_BOOLEAN },
    { M_getRuntime,                   "getRuntime",                   METHOD_TYPE_OBJECT },
    { M_availableProcessors,          "availableProcessors",          METHOD_TYPE_INT },
    { M_getAvailMemory,               "getAvailMemory",               METHOD_TYPE_LONG },
    { M_getClipboard,                 "getClipboard",                 METHOD_TYPE_OBJECT },
    { M_screencapture,                "screencapture",                METHOD_TYPE_OBJECT },
    { M_getEditText,                  "getEditText",                  METHOD_TYPE_OBJECT },

    { M_removeSystemFontDataAll,   "removeSystemFontDataAll",   METHOD_TYPE_VOID },
    { M_resumeCheckLicense,        "resumeCheckLicense",        METHOD_TYPE_VOID },
    { M_setClipboard,              "setClipboard",              METHOD_TYPE_VOID },
    { M_hideNavigationBar,         "hideNavigationBar",         METHOD_TYPE_VOID },
    { M_setKeepScreenOn,           "setKeepScreenOn",           METHOD_TYPE_VOID },
    { M_openURL,                   "openURL",                   METHOD_TYPE_VOID },
    { M_openTwitter,               "openTwitter",               METHOD_TYPE_VOID },
    { M_finish,                    "finish",                    METHOD_TYPE_VOID },
    { M_hideKeyInput,              "hideKeyInput",              METHOD_TYPE_VOID },
    { M_addView,                   "addView",                   METHOD_TYPE_VOID },
    { M_removeView,                "removeView",                METHOD_TYPE_VOID },
    { M_setViewVisibility,         "setViewVisibility",         METHOD_TYPE_VOID },
    { M_setTag,                    "setTag",                    METHOD_TYPE_VOID },
    { M_setBackgroundColor,        "setBackgroundColor",        METHOD_TYPE_VOID },
    { M_setString,                 "setString",                 METHOD_TYPE_VOID },
    { M_getTextBounds,             "getTextBounds",             METHOD_TYPE_VOID },
    { M_sendNotification,          "sendNotification",          METHOD_TYPE_VOID },
    { M_deleteNotification,        "deleteNotification",        METHOD_TYPE_VOID },
    { M_openImagePicker,           "openImagePicker",           METHOD_TYPE_VOID },
    { M_openQRCodeDecoder,         "openQRCodeDecoder",         METHOD_TYPE_VOID },
    { M_closeQRCodeDecoder,        "closeQRCodeDecoder",        METHOD_TYPE_VOID },
    { M_openQRCodeEncoder,         "openQRCodeEncoder",         METHOD_TYPE_VOID },
    { M_closeQRCodeEncoder,        "closeQRCodeEncoder",        METHOD_TYPE_VOID },
    { M_showInfoDialog,            "showInfoDialog",            METHOD_TYPE_VOID },
    { M_showSelectDialog,          "showSelectDialog",          METHOD_TYPE_VOID },
    { M_showProgressDialogBinding, "showProgressDialogBinding", METHOD_TYPE_VOID },
    { M_dissmissProgressDialog,    "dissmissProgressDialog",    METHOD_TYPE_VOID },
    { M_getAndroidVolume,          "getAndroidVolume",          METHOD_TYPE_VOID },
    { M_getAudioTrackBufferSize,   "getAudioTrackBufferSize",   METHOD_TYPE_VOID },
    { M_kill,                      "kill",                      METHOD_TYPE_VOID },

    { M_enumGamePadDevice,      "enumGamePadDevice",      METHOD_TYPE_VOID },
    { M_isInputDeviceAvailable, "isInputDeviceAvailable", METHOD_TYPE_BOOLEAN },
    { M_getInputDeviceName,     "getInputDeviceName",     METHOD_TYPE_OBJECT },

    { M_play,                   "play",                   METHOD_TYPE_VOID },
    { M_stop,                   "stop",                   METHOD_TYPE_VOID },
    { M_pause,                  "pause",                  METHOD_TYPE_VOID },
    { M_prepare,                "prepare",                METHOD_TYPE_VOID },
    { M_seek,                   "seek",                   METHOD_TYPE_VOID },
    { M_cleanUp,                "cleanUp",                METHOD_TYPE_VOID },
    { M_setAssetPath,           "setAssetPath",           METHOD_TYPE_VOID },
    { M_setExternalStoragePath, "setExternalStoragePath", METHOD_TYPE_VOID },
    { M_setSurface,             "setSurface",             METHOD_TYPE_VOID },
    { M_setVolume,              "setVolume",              METHOD_TYPE_VOID },
    { M_setSubtitle,            "setSubtitle",            METHOD_TYPE_VOID },
    { M_updateTexImage,         "updateTexImage",         METHOD_TYPE_VOID },
    { M_getTransformMatrix,     "getTransformMatrix",     METHOD_TYPE_VOID },
    { M_getDuration,            "getDuration",            METHOD_TYPE_INT },
    { M_getCurrentPosition,     "getCurrentPosition",     METHOD_TYPE_INT },
    { M_getVideoWidth,          "getVideoWidth",          METHOD_TYPE_INT },
    { M_getVideoHeight,         "getVideoHeight",         METHOD_TYPE_INT },

    { M_isSignedIn,                "isSignedIn",                METHOD_TYPE_BOOLEAN },
    { M_isConnecting,              "isConnecting",              METHOD_TYPE_BOOLEAN },
    { M_isSignedFailed,            "isSignedFailed",            METHOD_TYPE_BOOLEAN },
    { M_hasSignInError,            "hasSignInError",            METHOD_TYPE_BOOLEAN },
    { M_isOnline,                  "isOnline",                  METHOD_TYPE_BOOLEAN },
    { M_isConnected,               "isConnected",               METHOD_TYPE_BOOLEAN },
    { M_isSuccess,                 "isSuccess",                 METHOD_TYPE_BOOLEAN },
    { M_isResolveConflictFailed,   "isResolveConflictFailed",   METHOD_TYPE_BOOLEAN },
    { M_readSavedGames,            "readSavedGames",            METHOD_TYPE_BOOLEAN },
    { M_writeSavedGames,           "writeSavedGames",           METHOD_TYPE_BOOLEAN },
    { M_resolveConflict,           "resolveConflict",           METHOD_TYPE_BOOLEAN },
    { M_init,                      "init",                      METHOD_TYPE_BOOLEAN },
    { M_start,                     "start",                     METHOD_TYPE_BOOLEAN },
    { M_isOverLen,                 "isOverLen",                 METHOD_TYPE_BOOLEAN },
    { M_isSignedCancelled,         "isSignedCancelled",         METHOD_TYPE_BOOLEAN },
    { M_beginUserInitiatedSignIn,  "beginUserInitiatedSignIn",  METHOD_TYPE_VOID },
    { M_signOut,                   "signOut",                   METHOD_TYPE_VOID },
    { M_reconnectClient,           "reconnectClient",           METHOD_TYPE_VOID },
    { M_setMaxAutoSignInAttempts,  "setMaxAutoSignInAttempts",  METHOD_TYPE_VOID },
    { M_showSavedGamesUI,          "showSavedGamesUI",          METHOD_TYPE_VOID },
    { M_setUseServerVersion,       "setUseServerVersion",       METHOD_TYPE_VOID },
    { M_showAchievement,           "showAchievement",           METHOD_TYPE_VOID },
    { M_createPlayersClient,       "createPlayersClient",       METHOD_TYPE_VOID },
    { M_createRoom,                "createRoom",                METHOD_TYPE_VOID },
    { M_sendData,                  "sendData",                  METHOD_TYPE_VOID },
    { M_setLocalSessionId,         "setLocalSessionId",         METHOD_TYPE_VOID },
    { M_releaseSession,            "releaseSession",            METHOD_TYPE_VOID },
    { M_listen_close,              "listen_close",              METHOD_TYPE_VOID },
    { M_clearConnectWork,          "clearConnectWork",          METHOD_TYPE_VOID },
    { M_setSessionLock,            "setSessionLock",            METHOD_TYPE_VOID },
    { M_ensureBluetoothDiscoverable,"ensureBluetoothDiscoverable",METHOD_TYPE_VOID },
    { M_discoverBluetoothDevice,   "discoverBluetoothDevice",   METHOD_TYPE_VOID },
    { M_onStop,                    "onStop",                    METHOD_TYPE_VOID },
    { M_getStatusCode,             "getStatusCode",             METHOD_TYPE_INT },
    { M_loadAchievement,           "loadAchievement",           METHOD_TYPE_INT },
    { M_leaveRoom,                 "leaveRoom",                 METHOD_TYPE_INT },
    { M_getStartingMemberListNum,  "getStartingMemberListNum",  METHOD_TYPE_INT },
    { M_getDeviceNum,              "getDeviceNum",              METHOD_TYPE_INT },
    { M_getDeviceInfo,             "getDeviceInfo",             METHOD_TYPE_INT },
    { M_listen,                    "listen",                    METHOD_TYPE_INT },
    { M_connect,                   "connect",                   METHOD_TYPE_INT },
    { M_sendSessionData,           "sendSessionData",           METHOD_TYPE_INT },
    { M_getMaxDataSize,            "getMaxDataSize",            METHOD_TYPE_INT },
    { M_getApiClient,              "getApiClient",              METHOD_TYPE_OBJECT },
    { M_getSelfPlayerId,           "getSelfPlayerId",           METHOD_TYPE_OBJECT },
    { M_getStartingMemberPlayerId, "getStartingMemberPlayerId", METHOD_TYPE_OBJECT },
    { M_getBluetoothLocalAddress,  "getBluetoothLocalAddress",  METHOD_TYPE_OBJECT },
    { M_getConnectAddress,         "getConnectAddress",         METHOD_TYPE_OBJECT },
};

MethodsBoolean methodsBoolean[] = {
    { M_getBooleanParameter, getBooleanParameter },
    { M_getKeepScreenOn, getKeepScreenOn },
    { M_moveTaskToBack, moveTaskToBack },
    { M_isExternalStorageMounted, isExternalStorageMounted },
    { M_isExternalStorageRemovable, isExternalStorageRemovable },
    { M_createDir, createDir },
    { M_createDirRecursive, createDirRecursive },
    { M_isInputDeviceAvailable, isInputDeviceAvailable },
    { M_isSignedIn, ret_false },
    { M_isConnecting, ret_false },
    { M_isSignedFailed, ret_false },
    { M_hasSignInError, ret_false },
    { M_isOnline, ret_false },
    { M_isConnected, ret_false },
    { M_isSuccess, ret_false },
    { M_isResolveConflictFailed, ret_false },
    { M_readSavedGames, ret_false },
    { M_writeSavedGames, ret_false },
    { M_resolveConflict, ret_false },
    { M_init, ret_false },
    { M_start, ret_false },
    { M_isOverLen, ret_false },
    { M_isSignedCancelled, ret_true },
};
MethodsByte methodsByte[] = {};
MethodsChar methodsChar[] = {};
MethodsDouble methodsDouble[] = {};
MethodsFloat methodsFloat[] = {
    { M_getFloatParameter, getFloatParameter },
};
MethodsInt methodsInt[] = {
    { M_getParameterNum, getParameterNum },
    { M_getParameterType, getParameterType },
    { M_getIntegerParameter, getIntegerParameter },
    { M_getVersionCode, getVersionCode },
    { M_getMainOBBVersion, getMainOBBVersion },
    { M_getPatchOBBVersion, getPatchOBBVersion },
    { M_getLVLState, getLVLState },
    { M_getScreenOrientation, getScreenOrientation },
    { M_getDeviceRequestedOrientation, getDeviceRequestedOrientation },
    { M_getSystemFontDataNum, getSystemFontDataNum },
    { M_availableProcessors, availableProcessors },
    { M_getDuration, mp_getDuration },
    { M_getCurrentPosition, mp_getCurrentPosition },
    { M_getVideoWidth, mp_getVideoWidth },
    { M_getVideoHeight, mp_getVideoHeight },
    { M_getStatusCode, ret_minus1 },
    { M_loadAchievement, ret_zero },
    { M_leaveRoom, ret_zero },
    { M_getStartingMemberListNum, ret_zero },
    { M_getDeviceNum, ret_zero },
    { M_getDeviceInfo, ret_zero },
    { M_listen, ret_minus1 },
    { M_connect, ret_minus1 },
    { M_sendSessionData, ret_minus1 },
    { M_getMaxDataSize, ret_zero },
};
MethodsLong methodsLong[] = {
    { M_getLongParameter, getLongParameter },
    { M_getAvailMemory, getAvailMemory },
};
MethodsObject methodsObject[] = {
    { M_getEventName, getEventName },
    { M_getStringParameter, getStringParameter },
    { M_getObjectParameter, getObjectParameter },
    { M_getLanguage, getLanguage },
    { M_getVersionName, getVersionName },
    { M_getPackageName, getPackageName },
    { M_getByteArray, getByteArray },
    { M_getFileName, getFileName },
    { M_getSharedFileDir, getSharedFileDir },
    { M_getExternalStorageDirectory, getExternalStorageDirectory },
    { M_getExternalFilesDir, getExternalFilesDir },
    { M_getAbsolutePath, getAbsolutePath },
    { M_getPath, getPath },
    { M_getRuntime, getRuntime },
    { M_getClipboard, ret_empty_str },
    { M_screencapture, ret_null },
    { M_getEditText, ret_empty_str },
    { M_getInputDeviceName, getInputDeviceName },
    { M_getApiClient, ret_null },
    { M_getSelfPlayerId, ret_empty_str },
    { M_getStartingMemberPlayerId, ret_empty_str },
    { M_getBluetoothLocalAddress, ret_empty_str },
    { M_getConnectAddress, ret_empty_str },
};
MethodsShort methodsShort[] = {};
MethodsVoid methodsVoid[] = {
    { M_removeSystemFontDataAll, noop },
    { M_resumeCheckLicense, noop },
    { M_setClipboard, noop },
    { M_hideNavigationBar, noop },
    { M_setKeepScreenOn, noop },
    { M_openURL, noop },
    { M_openTwitter, noop },
    { M_finish, noop },
    { M_hideKeyInput, noop },
    { M_addView, noop },
    { M_removeView, noop },
    { M_setViewVisibility, noop },
    { M_setTag, noop },
    { M_setBackgroundColor, noop },
    { M_setString, noop },
    { M_getTextBounds, noop },
    { M_sendNotification, noop },
    { M_deleteNotification, noop },
    { M_openImagePicker, noop },
    { M_openQRCodeDecoder, noop },
    { M_closeQRCodeDecoder, noop },
    { M_openQRCodeEncoder, noop },
    { M_closeQRCodeEncoder, noop },
    { M_showInfoDialog, noop },
    { M_showSelectDialog, noop },
    { M_showProgressDialogBinding, noop },
    { M_dissmissProgressDialog, noop },
    { M_getAndroidVolume, noop },
    { M_getAudioTrackBufferSize, noop },
    { M_kill, noop },
    { M_enumGamePadDevice, enumGamePadDevice },
    { M_play, mp_play },
    { M_stop, mp_stop },
    { M_pause, mp_pause },
    { M_prepare, mp_prepare },
    { M_seek, mp_seek },
    { M_cleanUp, mp_cleanUp },
    { M_setAssetPath, mp_setAssetPath },
    { M_setExternalStoragePath, mp_setExternalStoragePath },
    { M_setSurface, mp_setSurface },
    { M_setVolume, mp_setVolume },
    { M_setSubtitle, mp_setSubtitle },
    { M_updateTexImage, mp_updateTexImage },
    { M_getTransformMatrix, mp_getTransformMatrix },
    { M_beginUserInitiatedSignIn, noop },
    { M_signOut, noop },
    { M_reconnectClient, noop },
    { M_setMaxAutoSignInAttempts, noop },
    { M_showSavedGamesUI, noop },
    { M_setUseServerVersion, noop },
    { M_showAchievement, noop },
    { M_createPlayersClient, noop },
    { M_createRoom, noop },
    { M_sendData, noop },
    { M_setLocalSessionId, noop },
    { M_releaseSession, noop },
    { M_listen_close, noop },
    { M_clearConnectWork, noop },
    { M_setSessionLock, noop },
    { M_ensureBluetoothDiscoverable, noop },
    { M_discoverBluetoothDevice, noop },
    { M_onStop, noop },
};

/* ------------------------------------------------------------------------ */
/* Fields                                                                   */
/* ------------------------------------------------------------------------ */

char WINDOW_SERVICE[] = "window";
const int SDK_INT = 23;                 /* Android 6.0 */
char BUILD_MODEL[] = "PS Vita";
char BUILD_MANUFACTURER[] = "Sony";
char BUILD_RELEASE[] = "6.0";

NameToFieldID nameToFieldId[] = {
    { 0, "WINDOW_SERVICE", FIELD_TYPE_OBJECT },
    { 1, "SDK_INT",        FIELD_TYPE_INT },
    { 2, "MODEL",          FIELD_TYPE_OBJECT },
    { 3, "MANUFACTURER",   FIELD_TYPE_OBJECT },
    { 4, "RELEASE",        FIELD_TYPE_OBJECT },
    { 5, "orientation",    FIELD_TYPE_INT },
};

FieldsBoolean fieldsBoolean[] = {};
FieldsByte fieldsByte[] = {};
FieldsChar fieldsChar[] = {};
FieldsDouble fieldsDouble[] = {};
FieldsFloat fieldsFloat[] = {};
FieldsInt fieldsInt[] = {
    { 1, SDK_INT },
    { 5, 2 },                           /* Configuration.ORIENTATION_LANDSCAPE */
};
FieldsObject fieldsObject[] = {
    { 0, WINDOW_SERVICE },
    { 2, BUILD_MODEL },
    { 3, BUILD_MANUFACTURER },
    { 4, BUILD_RELEASE },
};
FieldsLong fieldsLong[] = {};
FieldsShort fieldsShort[] = {};

__FALSOJNI_IMPL_CONTAINER_SIZES
