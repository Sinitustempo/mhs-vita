/*
 * Monster Hunter Stories (Android 1.0.5) -> PS Vita
 *
 * Builds MTFPEvent look-alikes and delivers them to libMHR.so. Each sender
 * mirrors the Java code in MTFPActivity that produces the same event.
 */

#include "mhs/mtfp_event.h"
#include "mhs/mhs_native.h"

#include <string.h>
#include <so_util/so_util.h>

#include "utils/logger.h"

MtfpEvent *mtfp_current_event = NULL;

/* Android InputDevice id we report for the built-in Vita controls. */
#define VITA_PAD_DEVICE_ID 1

void mtfp_event_init(MtfpEvent *ev, const char *name, int num) {
    memset(ev, 0, sizeof(*ev));
    ev->name = name;
    ev->num = num > MTFP_EVENT_MAX_PARAMS ? MTFP_EVENT_MAX_PARAMS : num;
    for (int i = 0; i < MTFP_EVENT_MAX_PARAMS; i++)
        ev->params[i].type = MTFP_PARAM_INVALID;
}

void mtfp_event_set_int(MtfpEvent *ev, int idx, int32_t v) {
    if (idx < 0 || idx >= ev->num) return;
    ev->params[idx].type = MTFP_PARAM_INTEGER;
    ev->params[idx].i = v;
}

void mtfp_event_set_float(MtfpEvent *ev, int idx, float v) {
    if (idx < 0 || idx >= ev->num) return;
    ev->params[idx].type = MTFP_PARAM_FLOAT;
    ev->params[idx].f = v;
}

bool mtfp_event_send(MtfpEvent *ev) {
    /* The game reads the event back synchronously through JNI calls that
     * java.c answers from mtfp_current_event, so it only has to live for the
     * duration of this call. Everything runs on the main thread. */
    mtfp_current_event = ev;
    jboolean ret = mhs.notifyEvent(&jni, MHS_FAKE_CLASS, (jobject)ev);
    mtfp_current_event = NULL;
    return ret != JNI_FALSE;
}

/* MTFPActivity.dispatchKeyEvent() for SOURCE_GAMEPAD / SOURCE_JOYSTICK keys */
void mtfp_send_gamepad_key(int keycode, bool down) {
    MtfpEvent ev;
    mtfp_event_init(&ev, "ActivityGamePad", GamePadButtonParameterNum);
    mtfp_event_set_int(&ev, GamePadEventDeviceId, VITA_PAD_DEVICE_ID);
    mtfp_event_set_int(&ev, GamePadEventType, down ? GamePadDigitalKeyDown : GamePadDigitalKeyUp);
    mtfp_event_set_int(&ev, GamePadButtonCode, keycode);
    mtfp_event_send(&ev);
}

/* MTFPActivity.dispatchGenericMotionEvent() for joystick motion.
 * Android axis conventions: sticks in [-1, 1] with +Y down; Z/RZ is the right
 * stick on most pads (AXIS_Z = 11, AXIS_RZ = 14), triggers in [0, 1]. */
void mtfp_send_gamepad_axes(float lx, float ly, float rx, float ry,
                            float hat_x, float hat_y, float lt, float rt) {
    MtfpEvent ev;
    mtfp_event_init(&ev, "ActivityGamePad", GamePadAxisParameterNum);
    mtfp_event_set_int(&ev, GamePadEventDeviceId, VITA_PAD_DEVICE_ID);
    mtfp_event_set_int(&ev, GamePadEventType, GamePadAnalogStick);
    mtfp_event_set_float(&ev, GamePadAxisX, lx);
    mtfp_event_set_float(&ev, GamePadAxisY, ly);
    mtfp_event_set_float(&ev, GamePadAxisZ, rx);
    mtfp_event_set_float(&ev, GamePadAxisRX, 0.0f);
    mtfp_event_set_float(&ev, GamePadAxisRY, 0.0f);
    mtfp_event_set_float(&ev, GamePadAxisRZ, ry);
    mtfp_event_set_float(&ev, GamePadAxisHatX, hat_x);
    mtfp_event_set_float(&ev, GamePadAxisHatY, hat_y);
    mtfp_event_set_float(&ev, GamePadAxisLT, lt);
    mtfp_event_set_float(&ev, GamePadAxisRT, rt);
    mtfp_event_set_float(&ev, GamePadAxisGas, rt);
    mtfp_event_set_float(&ev, GamePadAxisBrake, lt);
    mtfp_event_send(&ev);
}

/* MTFPActivity.notifyGesture(type, x, y, pointerId, pointerCount, pointerIndex).
 * Slot order confirmed from onSingleTapConfirmed / onLongPress, which pass
 * (type, getX(), getY() - rect.top, getPointerId(0), 1, 0). */
void mtfp_send_gesture(int type, float x, float y, int pointer_id,
                       int pointer_count, int pointer_index) {
    MtfpEvent ev;
    mtfp_event_init(&ev, "ActivityGesture", 6);
    mtfp_event_set_int(&ev, 0, type);
    mtfp_event_set_float(&ev, 1, x);
    mtfp_event_set_float(&ev, 2, y);
    mtfp_event_set_int(&ev, 3, pointer_id);
    mtfp_event_set_int(&ev, 4, pointer_count);
    mtfp_event_set_int(&ev, 5, pointer_index);
    mtfp_event_send(&ev);
}

/* MTFPActivity.onKeyUp(KEYCODE_BACK): one parameter, left unset in Java. */
void mtfp_send_back_key(void) {
    MtfpEvent ev;
    mtfp_event_init(&ev, "ActivityOnBackKey", 1);
    mtfp_event_send(&ev);
}
