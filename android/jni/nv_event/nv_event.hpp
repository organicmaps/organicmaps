//----------------------------------------------------------------------------------
// File:            libs\jni\nv_event\nv_event.h
// Samples Version: NVIDIA Android Lifecycle samples 1_0beta 
// Email:           tegradev@nvidia.com
// Web:             http://developer.nvidia.com/category/zone/mobile-development
//
// Copyright 2009-2011 NVIDIA® Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//----------------------------------------------------------------------------------
#ifndef _NV_EVENT_H
#define _NV_EVENT_H
#include <jni.h>
#include <sys/types.h>
//#include <EGL/egl.h>

/** @file nv_event.h
Contains a framework for event loop-based applications.  This library replaces
most or all of the normally-required JNI code for Android NDK applications,
presenting the application developer with two callbacks into which they can
write their application.  The framework runs in a natively-created thread,
allowing the application to implement a classic "event loop and render" structure
without having to return to Java code to avoid ANR warnings.  The library 
includes support for input and system event passing as well as JNI initialization
and exposes basic EGL functionality to native as well.  Put together, the library
can form the basis of a simple interactive 3D application.  All applications that
are based on this library must also be subclassed on the Java side from the
provided NvEventQueueActivity.  Additional external documentation on the use of
this library, the related Java code and the tool provided to create a template
application based on the library are provided with the SDK.
@see NvEventQueueActivity
*/

#ifdef ANDROID

#include <jni.h>
typedef jobject NVEventPlatformAppHandle;

#else // unknown platform

typedef void* NVEventPlatformAppHandle;

#endif


enum
{
  /** Timeout argument to NVEventGetNextEvent() that indicates the function should
  block until there is an event pending or the app exits
  @see NVEventGetNextEvent()
  */
  NV_EVENT_WAIT_FOREVER = -1
};

/** Event type values
*/
typedef enum NVEventType
{
  /** Key up/down events */
  NV_EVENT_KEY = 1,
  /** Translated character events */
  NV_EVENT_CHAR,
  /** Single-touch pointer events */
  NV_EVENT_TOUCH,
  /** Multi-touch events */
  NV_EVENT_MULTITOUCH,
  /** Long Click Event */
  NV_EVENT_LONG_CLICK,
  /** Accelerometer events */
  NV_EVENT_ACCEL,
  /** onStart lifecycle events */
  NV_EVENT_START,
  /** onRestart lifecycle events */
  NV_EVENT_RESTART,
  /** onResume lifecycle events */
  NV_EVENT_RESUME,
  /** onWindowFocusChanged(TRUE) lifecycle events */
  NV_EVENT_FOCUS_GAINED,
  /** Surface created events */
  NV_EVENT_SURFACE_CREATED,
  /** Surface size changed events */
  NV_EVENT_SURFACE_SIZE,
  /** Surface destroyed events */
  NV_EVENT_SURFACE_DESTROYED,
  /** onWindowFocusChanged(FALSE) lifecycle events */
  NV_EVENT_FOCUS_LOST,
  /** onPause lifecycle events */
  NV_EVENT_PAUSE,
  /** onStop lifecycle events */
  NV_EVENT_STOP,
  /** (onDestroy) Quit request events */
  NV_EVENT_QUIT,
  /** App-specific events */
  NV_EVENT_USER,
  NV_EVENT_MWM,
  /* a dummy enum value used to compute num_events */
  NV_EVENT_NUM_EVENT_DUMMY_DONTUSEME,
  /* total number of events */
  NV_EVENT_NUM_EVENTS = NV_EVENT_NUM_EVENT_DUMMY_DONTUSEME - 1,
  NV_EVENT_FORCE_32BITS = 0x7fffffff
} NVEventType;

/** Touch event actions
*/
typedef enum NVTouchEventType
{
  /** Pointer has just left the screen */
  NV_TOUCHACTION_UP,
  /** Pointer has just gone down onto the screen */
  NV_TOUCHACTION_DOWN,
  /** Pointer is moving on the screen */
  NV_TOUCHACTION_MOVE,
  NV_TOUCHACTION_FORCE_32BITS = 0x7fffffff
} NVTouchEventType;

/** Multitouch event flags 
*/
typedef enum NVMultiTouchEventType
{
  /** Indicated pointers are leaving the screen */
  NV_MULTITOUCH_UP    =   0x00000001,
  /** Indicated pointers have just touched the screen */
  NV_MULTITOUCH_DOWN  =   0x00000002,
  /** Indicated pointers are moving on the screen */
  NV_MULTITOUCH_MOVE  =   0x00000003,
  /** Indicated pointers have halted the current gesture
      app should cancel any actions implied by the gesture */
  NV_MULTITOUCH_CANCEL =  0x00000004,
  /** Mask to be AND'ed with the flag value
      to get the active pointer bits */
  NV_MULTITOUCH_POINTER_MASK =  0x0000ff00,
  /** Number of bits to right-shift the masked value
      to get the active pointer bits */
  NV_MULTITOUCH_POINTER_SHIFT = 0x00000008,
  /** Mask to be AND'ed with the flag value
      to get the event action */
  NV_MULTITOUCH_ACTION_MASK =   0x000000ff,
  NV_MULTITOUCH_FORCE_32BITS = 0x7fffffff
} NVMultiTouchEventType;

/** Key event types
*/
typedef enum NVKeyEventType
{
  /** Key has just been pressed (no repeats) */
  NV_KEYACTION_UP,
  /** Key has just been released */
  NV_KEYACTION_DOWN,
  NV_KEYACTION_FORCE_32BITS = 0x7fffffff
} NVKeyEventType;

/** Key event key codes
*/
#define NV_MAX_KEYCODE 256
typedef enum NVKeyCode
{
  NV_KEYCODE_NULL = 0,
  NV_KEYCODE_BACK,
  NV_KEYCODE_TAB,
  NV_KEYCODE_ENTER,
  NV_KEYCODE_DEL,
  NV_KEYCODE_SPACE,
  NV_KEYCODE_ENDCALL,
  NV_KEYCODE_HOME,

  NV_KEYCODE_STAR,
  NV_KEYCODE_PLUS,
  NV_KEYCODE_MINUS,
  NV_KEYCODE_NUM,

  NV_KEYCODE_DPAD_LEFT,
  NV_KEYCODE_DPAD_UP,
  NV_KEYCODE_DPAD_RIGHT,
  NV_KEYCODE_DPAD_DOWN,

  NV_KEYCODE_0,
  NV_KEYCODE_1,
  NV_KEYCODE_2,
  NV_KEYCODE_3,
  NV_KEYCODE_4,
  NV_KEYCODE_5,
  NV_KEYCODE_6,
  NV_KEYCODE_7,
  NV_KEYCODE_8,
  NV_KEYCODE_9,

  NV_KEYCODE_A,
  NV_KEYCODE_B,
  NV_KEYCODE_C,
  NV_KEYCODE_D,
  NV_KEYCODE_E,
  NV_KEYCODE_F,
  NV_KEYCODE_G,
  NV_KEYCODE_H,
  NV_KEYCODE_I,
  NV_KEYCODE_J,
  NV_KEYCODE_K,
  NV_KEYCODE_L,
  NV_KEYCODE_M,
  NV_KEYCODE_N,
  NV_KEYCODE_O,
  NV_KEYCODE_P,
  NV_KEYCODE_Q,
  NV_KEYCODE_R,
  NV_KEYCODE_S,
  NV_KEYCODE_T,
  NV_KEYCODE_U,
  NV_KEYCODE_V,
  NV_KEYCODE_W,
  NV_KEYCODE_X,
  NV_KEYCODE_Y,
  NV_KEYCODE_Z,

  NV_KEYCODE_ALT_LEFT,
  NV_KEYCODE_ALT_RIGHT,

  NV_KEYCODE_SHIFT_LEFT,
  NV_KEYCODE_SHIFT_RIGHT,

  NV_KEYCODE_APOSTROPHE,
  NV_KEYCODE_SEMICOLON,
  NV_KEYCODE_EQUALS,
  NV_KEYCODE_COMMA,
  NV_KEYCODE_PERIOD,
  NV_KEYCODE_SLASH,
  NV_KEYCODE_GRAVE,
  NV_KEYCODE_BACKSLASH,

  NV_KEYCODE_LEFT_BRACKET,
  NV_KEYCODE_RIGHT_BRACKET,

  NV_KEYCODE_FORCE_32BIT = 0x7fffffff
} NVKeyCode;

/** Single-touch event data
*/
typedef struct NVEventTouch
{
  /** The action code */
  NVTouchEventType   m_action;
  /** The window-relative X position (in pixels) */
  float   m_x;
  /** The window-relative Y position (in pixels) */
  float   m_y;
} NVEventTouch;

/** Multi-touch event data
*/
typedef struct NVEventMultiTouch
{
  /** The action flags */
  NVMultiTouchEventType   m_action;
  /** The window-relative X position of the first pointer (in pixels)
      only valid if bit 0 of the pointer bits is set */
  float   m_x1;
  /** The window-relative Y position of the first pointer (in pixels)
      only valid if bit 0 of the pointer bits is set */
  float   m_y1;
  /** The window-relative X position of the second pointer (in pixels)
      only valid if bit 1 of the pointer bits is set */
  float   m_x2;
  /** The window-relative Y position of the second pointer (in pixels)
      only valid if bit 1 of the pointer bits is set */
  float   m_y2;
} NVEventMultiTouch;

/** Key down/up event data
*/
typedef struct NVEventKey
{
  /** The action flags */
  NVKeyEventType m_action;
  /** The code of the key pressed or released */
  NVKeyCode m_code;
} NVEventKey;

/** Translated character event data
*/
typedef struct NVEventChar
{
  /** The UNICODE character represented */
  int32_t m_unichar;
} NVEventChar;

/** Accelerometer event data
*/
typedef struct NVEventAccel
{
  /** Signed X magnitude of the force vector */
  float   m_x;
  /** Signed Y magnitude of the force vector */
  float   m_y;
  /** Signed Z magnitude of the force vector */
  float   m_z;
} NVEventAccel;

/** Surface size change event data
*/
typedef struct NVEventSurfaceSize
{
  /** New surface client area width (in pixels) */
  int32_t m_w;
  /** New surface client area height (in pixels) */
  int32_t m_h;
  /** Screen density for the screen at which this surface was created*/
  int32_t m_density;
} NVEventSurfaceSize;

/** User/App-specific event data
*/
typedef struct NVEventUser
{
  /** First 32-bit user data item */
  int32_t m_u0;
  /** Second 32-bit user data item */
  int32_t m_u1;
  /** Third 32-bit user data item */
  int32_t m_u2;
  /** Fourth 32-bit user data item */
  int32_t m_u3;
} NVEventUser;

typedef struct NVEventMWM
{
  void * m_pFn;
} NVEventMWM;

/** All-encompassing event structure 
*/
typedef struct NVEvent 
{
  /** The type of the event, which also indicates which m_data union holds the data */
  NVEventType m_type;
  /** Union containing all possible event type data */
  union NVEventData
  {
    /** Data for single-touch events */
    NVEventTouch m_touch;
    /** Data for multi-touch events */
    NVEventMultiTouch m_multi;
    /** Data for key up/down events */
    NVEventKey m_key;
    /** Data for charcter events */
    NVEventChar m_char;
    /** Data for accelerometer events */
    NVEventAccel m_accel;
    /** Data for surface size events */
    NVEventSurfaceSize m_size;
    /** Data for user/app events */
    NVEventUser m_user;
    NVEventMWM m_mwm;
  } m_data;
} NVEvent;

/** Returns a string describing the event
@param eventType The event type
@return Returns a string containing a description of the event. Do not free or delete this memory.
@see NVEvent */
const char* NVEventGetEventStr(NVEventType eventType);

/** Returns the next pending event for the application to process.  Can return immediately if there
is no event, or can wait a fixed number of milisecs (or "forever") if desired.
The application should always pair calls to this function that return non-NULL events with calls 
to NVEventDoneWithEvent()
@param waitMSecs The maximum time (in milisecs) to wait for an event before returning "no event".  
	Pass NV_EVENT_WAIT_FOREVER to wait indefinitely for an event.  Note that NV_EVENT_WAIT_FOREVER
	does not guarantee an event on return.  The function can still return on error or if the
	app is exiting. Default is to return immediately, event or not. 
@return Non-NULL pointer to a constant event structure if an event was pending, NULL if no event was
	pending in the requested timeout period
@see NVEvent 
@see NVEventDoneWithEvent 
*/
const NVEvent* NVEventGetNextEvent(int waitMSecs = 0);

/** Indicates that the application has finished handling the event returned from the last 
call to NVEventGetNextEvent.  This function should always be called prior to the next call 
to NVEventGetNextEvent.  If the current event is a blocking event, this call will unblock 
the posting thread (normally in Java).  This is particularly important for application 
lifecycle events like onPause, as calling this function indicates that the native code has
completed the handling of the lifecycle callback.  Failure to call this function promptly 
for all events can lead to Application Not Responding errors.
@param handled The return value that should be passed back to Java for blocking events.  For non-blocking 
events, this parameter is discard.
@see NVEvent 
@see NVEventGetNextEvent 
*/
void NVEventDoneWithEvent(bool handled);

/** The app-supplied "callback" for initialization during JNI_OnLoad.  
Declares the application's pre-main initialization function.  Does not define the
function.  <b>The app must define this in its own code</b>, even if the function is empty.
JNI init code can be safely called from here, as it WILL be called from
within a JNI function thread
@parm argc Passes the number of command line arguments.  
	This is currently unsupported and is always passed 0 
@parm argv Passes the array of command line arguments.  
	This is currently unsupported and is always passed NULL 
@return The function should return 0 on success and nonzero on failure.
*/
extern int32_t NVEventAppInit(int32_t argc, char** argv);

/** The app-supplied "callback" for running the application's main loop.  
Declares the application's main loop function.  Does not define the
function.  <b>The app must define this in its own code</b>
This function will be spawned in its own thread. 
@parm argc Passes the number of command line arguments.  
	This is currently unsupported and is always passed 0 
@parm argv Passes the array of command line arguments.  
	This is currently unsupported and is always passed NULL 
@return The function should return 0 on success and nonzero on failure.
*/
extern int32_t NVEventAppMain(int32_t argc, char** argv);


/** Initializes EGL, queries a valid ES1 config and creates (but does not bind)
 an ES1-compatible EGLContext
@return true on success, false on failure
*/
bool NVEventInitEGL();

/** Releases any existing EGLSurface and EGLContext and terminates EGL
@return true on success, false on failure
*/
bool NVEventCleanupEGL();

/** Creates an EGLSurface for the current Android surface.  Will attempt to initialize 
 EGL and an EGLContext if not already done.  Fails if there is no valid Android surface 
 or if there is an EGL error.
@return true on success, false on failure
*/
bool NVEventCreateSurfaceEGL();

/** Unbinds (if needed) and releases the app's EGLSurface
@return true on success, false on failure
*/
bool NVEventDestroySurfaceEGL();

/** Binds the app's EGLSurface and EGLContext to the calling thread.  The EGLSurface and
 EGLContext  must both exist already, or else the function will fail.
@return true on success, false on failure
*/
bool NVEventBindSurfaceAndContextEGL();

/** Un-binds the app's EGLSurface and EGLContext from the calling thread.
@return true on success, false on failure
*/
bool NVEventUnbindSurfaceAndContextEGL();

/** Swaps the currently-bound EGLSurface
@return true on success, false on failure
*/
bool NVEventSwapBuffersEGL();
    
/** Accessor for the last EGL error
@return the EGL error
*/
int NVEventGetErrorEGL();

/** Utility function: checks if EGl is completely ready to render, including
 initialization, surface creation and context/surface binding. 
@parm allocateIfNeeded If the parameter is false, then the function immediately returns if any
 part of the requirements have not already been satisfied.  If the parameter is
 true, then the function attempts to initialize any of the steps needed, failing
 and returning false only if a step cannot be completed at this time. 
@return The function returns true if EGL/GLES is ready to render/load content (i.e.
 a context and surface are bound) and false it not
*/
bool NVEventReadyToRenderEGL(bool allocateIfNeeded);

/** Convenience conditional function to determine if the app is between onCreate 
 and onDestroy callbacks (i.e. the app is in a running state).
@return true if the application is between onCreate and onDestroy and false after 
 an onDestroy event has been delivered.
*/
bool NVEventStatusIsRunning();

/** Convenience conditional function to determine if the app is between onResume and onPause
 callbacks (i.e. the app is in a "resumed" state).
@return true if the application is between onResume and onPause and false if the application
 has not yet been resumed, or is currently paused.
*/
bool NVEventStatusIsActive();

/** Convenience conditional function to determine if the app's window is focused (between
 calls to onWindowFocusChanged(true) and onWindowFocusChanged(false))
@return true between onWindowFocusChanged(true) and onWindowFocusChanged(false)
*/
bool NVEventStatusIsFocused();

/** Convenience conditional function to determine if the app has a surface and that surface 
 has non-zero area
@return true if the app is between surfaceCreated and surfaceDestroyed callbacks and 
 the surface has non-zero pixel area (not 0x0 pixels)
*/
bool NVEventStatusHasRealSurface();

/** Convenience conditional function to determine if the app is in a fully-focused, visible 
 state.  This is a logical "AND" of IsRunning, IsActive, IsFocused and HasRealSurface
@return true if IsRunning, IsActive, IsFocused and HasRealSurface are all currently true, 
false otherwise
*/
bool NVEventStatusIsInteractable();

/** Convenience conditional function to determine if the app has active EGL 
@return true between successful calls to NVEventInitEGL and NVEventCleanupEGL, 
false otherwise
*/
bool NVEventStatusEGLInitialized();

/** Convenience conditional function to determine if the app has an EGLSurface (need not be bound) 
@return true if the EGLSurface for the app is allocated, false otherwise
*/
bool NVEventStatusEGLHasSurface();

/** Convenience conditional function to determine if the app has an EGLSurface and EGLContext 
 and they are bound 
@return true if the EGLSurface and EGLContext for the app are allocated and bound, false otherwise
*/
bool NVEventStatusEGLIsBound();

bool NVEventRepaint();

void NVEventReportUnsupported();

void NVEventOnRenderingInitialized();


/** Returns the platform-specific handle to the application instance, if supported.  This
function is, by definition platform-specific.
@return A platform-specific handle to the application.  */
NVEventPlatformAppHandle NVEventGetPlatformAppHandle();

void InitNVEvent(JavaVM * jvm);

void postMWMEvent(void * pFn, bool blocking);

#endif
