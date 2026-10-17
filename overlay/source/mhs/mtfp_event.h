/*
 * Monster Hunter Stories (Android 1.0.5) -> PS Vita
 *
 * Fake jp.co.capcom.mhsspen.MTFPEvent objects.
 *
 * On Android, MTFPActivity builds an MTFPEvent (name + typed parameter array)
 * and hands it to libMHR.so through the native MTFPJNI.notifyEvent(). The
 * native side then reads it back with getEventName / getParameterNum /
 * getParameterType / get*Parameter JNI calls. We build the same thing in C and
 * answer those calls from java.c.
 *
 * Layouts below were recovered from classes.dex of the 1.0.5 APK.
 */

#ifndef MHS_MTFP_EVENT_H
#define MHS_MTFP_EVENT_H

#include <stdint.h>
#include <stdbool.h>

/* MTFPEvent.getParameterType() return codes (MTFPEvent static fields e..j) */
typedef enum {
    MTFP_PARAM_INVALID = -1,
    MTFP_PARAM_BOOLEAN = 0,
    MTFP_PARAM_INTEGER = 1,
    MTFP_PARAM_FLOAT   = 2,
    MTFP_PARAM_STRING  = 3,
    MTFP_PARAM_OBJECT  = 4,
    MTFP_PARAM_LONG    = 5,
} MtfpParamType;

/* MTFPJNI static constants (from the dex static_values table) */
enum {
    /* "ActivityGamePad" parameter slots */
    GamePadEventDeviceId      = 0,
    GamePadEventType          = 1,
    GamePadButtonCode         = 2,
    GamePadButtonParameterNum = 3,
    GamePadAxisX              = 2,
    GamePadAxisY              = 3,
    GamePadAxisZ              = 4,
    GamePadAxisRX             = 5,
    GamePadAxisRY             = 6,
    GamePadAxisRZ             = 7,
    GamePadAxisHatX           = 8,
    GamePadAxisHatY           = 9,
    GamePadAxisLT             = 10,
    GamePadAxisRT             = 11,
    GamePadAxisGas            = 12,
    GamePadAxisBrake          = 13,
    GamePadAxisParameterNum   = 14,

    /* values for the GamePadEventType slot */
    GamePadDigitalKeyDown = 0,
    GamePadDigitalKeyUp   = 1,
    GamePadAnalogStick    = 2,

    /* "ActivityGesture" type values (slot 0) */
    GestureOnDown               = 0,
    GestureOnShowPress          = 1,
    GestureOnLongPress          = 2,
    GestureOnFling              = 3,
    GestureOnScroll             = 4,
    GestureOnSingleTapUp        = 5,
    GestureOnSingleTapConfirmed = 6,
    GestureOnDoubleTap          = 7,
    GestureOnDoubleTapEvent     = 8,
    GestureOnScaleBegin         = 9,
    GestureOnScaleEnd           = 10,
    GestureOnScale              = 11,
    GestureOnTouchRelease       = 12,
};

#define MTFP_EVENT_MAX_PARAMS 16

typedef struct {
    MtfpParamType type;
    union {
        bool b;
        int32_t i;
        float f;
        int64_t l;
        const char *s;
        void *o;
    };
} MtfpParam;

typedef struct {
    const char *name;
    int num;
    MtfpParam params[MTFP_EVENT_MAX_PARAMS];
} MtfpEvent;

/* The event currently being delivered through notifyEvent(); java.c reads it. */
extern MtfpEvent *mtfp_current_event;

void mtfp_event_init(MtfpEvent *ev, const char *name, int num);
void mtfp_event_set_int(MtfpEvent *ev, int idx, int32_t v);
void mtfp_event_set_float(MtfpEvent *ev, int idx, float v);

/* Delivers ev to libMHR.so's notifyEvent(). Returns the game's result. */
bool mtfp_event_send(MtfpEvent *ev);

/* Convenience senders mirroring MTFPActivity's Java code. */
void mtfp_send_gamepad_key(int keycode, bool down);
void mtfp_send_gamepad_axes(float lx, float ly, float rx, float ry,
                            float hat_x, float hat_y, float lt, float rt);
void mtfp_send_gesture(int type, float x, float y, int pointer_id,
                       int pointer_count, int pointer_index);
void mtfp_send_back_key(void);

#endif
