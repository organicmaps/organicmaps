package app.organicmaps.sdk;

import android.content.Context;
import android.graphics.Rect;
import android.view.MotionEvent;
import android.view.Surface;
import androidx.annotation.IntDef;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.display.DisplayType;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.util.Assert;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.ROMUtils;
import app.organicmaps.sdk.util.Utils;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.sdk.util.log.Logger;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public final class Map
{
  public interface CallbackUnsupported
  {
    void report();
  }

  @NonNull
  private static final String TAG = Map.class.getSimpleName();

  // Should correspond to android::MultiTouchAction from Framework.cpp
  @Retention(RetentionPolicy.SOURCE)
  @IntDef({MultiTouchAction.UP, MultiTouchAction.DOWN, MultiTouchAction.MOVE, MultiTouchAction.CANCEL})
  @interface MultiTouchAction
  {
    int UP = 1;
    int DOWN = 2;
    int MOVE = 3;
    int CANCEL = 4;
  }

  @Retention(RetentionPolicy.SOURCE)
  @IntDef(value = {PointerIndex.INVALID, PointerIndex.FIRST, PointerIndex.INVALID_MASKED_POINTER}, open = true)
  @interface PointerIndex
  {
    int INVALID = -1;
    int FIRST = 0;
    // Should correspond to df::TouchEvent::INVALID_MASKED_POINTER from user_event_stream.cpp
    int INVALID_MASKED_POINTER = 0xFF;
  }

  // Should correspond to gui::EWidget from skin.hpp
  @Retention(RetentionPolicy.SOURCE)
  @IntDef({WidgetType.RULER, WidgetType.COMPASS, WidgetType.COPYRIGHT, WidgetType.SCALE_FPS_LABEL})
  private @interface WidgetType
  {
    int RULER = 0x01;
    int COMPASS = 0x02;
    int COPYRIGHT = 0x04;
    int SCALE_FPS_LABEL = 0x08;
  }

  // Should correspond to dp::Anchor from drape_global.hpp
  @Retention(RetentionPolicy.SOURCE)
  @IntDef(value = {Anchor.CENTER, Anchor.LEFT, Anchor.RIGHT, Anchor.TOP, Anchor.BOTTOM, Anchor.LEFT_TOP,
                   Anchor.RIGHT_TOP, Anchor.LEFT_BOTTOM, Anchor.RIGHT_BOTTOM},
          flag = true)
  private @interface Anchor
  {
    int CENTER = 0x00;
    int LEFT = 0x01;
    int RIGHT = (LEFT << 1);
    int TOP = (RIGHT << 1);
    int BOTTOM = (TOP << 1);
    int LEFT_TOP = (LEFT | TOP);
    int RIGHT_TOP = (RIGHT | TOP);
    int LEFT_BOTTOM = (LEFT | BOTTOM);
    int RIGHT_BOTTOM = (RIGHT | BOTTOM);
  }

  @NonNull
  private final DisplayType mDisplayType;

  @Nullable
  private LocationHelper mLocationHelper;

  private int mCurrentCompassOffsetX;
  private int mCurrentCompassOffsetY;
  private int mBottomWidgetOffsetX;
  private int mBottomWidgetOffsetY;

  private int mHeight;
  private int mWidth;
  private boolean mRequireResize;
  private boolean mSurfaceCreated;
  private boolean mSurfaceAttached;
  private boolean mLaunchByDeepLink;
  @Nullable
  private MapRenderingListener mMapRenderingListener;
  @Nullable
  private CallbackUnsupported mCallbackUnsupported;

  private static int sCurrentDpi = 0;

  public Map(@NonNull DisplayType mapType)
  {
    mDisplayType = mapType;
    onCreate(false);
  }

  public void setLocationHelper(@NonNull LocationHelper locationHelper)
  {
    mLocationHelper = locationHelper;
  }

  /**
   * Moves the map compass using the given offsets.
   *
   * @param context     Context.
   * @param offsetX     Pixel offset from the top. -1 to keep the previous value.
   * @param offsetY     Pixel offset from the right.  -1 to keep the previous value.
   * @param forceRedraw True to force the compass to redraw
   */
  public void updateCompassOffset(@NonNull Context context, int offsetX, int offsetY, boolean forceRedraw)
  {
    final int x = offsetX < 0 ? mCurrentCompassOffsetX : offsetX;
    final int y = offsetY < 0 ? mCurrentCompassOffsetY : offsetY;
    final int navPadding = Utils.dimen(context, R.dimen.nav_frame_padding);
    final int marginX = Utils.dimen(context, R.dimen.margin_compass) + navPadding;
    final int marginY = Utils.dimen(context, R.dimen.margin_compass_top) + navPadding;
    nativeSetupWidget(WidgetType.COMPASS, mWidth - x - marginX, y + marginY, Anchor.CENTER);
    if (forceRedraw && mSurfaceCreated)
      nativeApplyWidgets();
    mCurrentCompassOffsetX = x;
    mCurrentCompassOffsetY = y;
  }

  public static void onCompassUpdated(double north, boolean forceRedraw)
  {
    nativeCompassUpdated(north, forceRedraw);
  }

  /**
   * Moves the ruler and copyright using the given offsets.
   *
   * @param context Context.
   * @param offsetX Pixel offset from the left.  -1 to keep the previous value.
   * @param offsetY Pixel offset from the bottom. -1 to keep the previous value.
   */
  public void updateBottomWidgetsOffset(@NonNull Context context, int offsetX, int offsetY)
  {
    final int x = offsetX < 0 ? mBottomWidgetOffsetX : offsetX;
    final int y = offsetY < 0 ? mBottomWidgetOffsetY : offsetY;
    updateRulerOffset(context, x, y);
    updateAttributionOffset(context, x, y);
    mBottomWidgetOffsetX = x;
    mBottomWidgetOffsetY = y;
  }

  /**
   * Moves my position arrow to the given offset.
   *
   * @param offsetY Pixel offset from the bottom.
   */
  public void updateMyPositionRoutingOffset(int offsetY)
  {
    nativeUpdateMyPositionRoutingOffset(offsetY);
  }

  public void onSurfaceCreated(@NonNull Context context, @NonNull Surface surface, @NonNull Rect surfaceFrame,
                               int surfaceDpi)
  {
    Assert.debug(mLocationHelper != null, "LocationHelper must be initialized before calling onSurfaceCreated");

    Logger.d(TAG, "mSurfaceCreated = " + mSurfaceCreated);
    if (nativeIsEngineCreated())
    {
      if (sCurrentDpi != surfaceDpi)
      {
        nativeUpdateEngineDpi(surfaceDpi);
        sCurrentDpi = surfaceDpi;

        setupWidgets(context, surfaceFrame.width(), surfaceFrame.height());
      }
      if (!nativeAttachSurface(surface))
      {
        if (mCallbackUnsupported != null)
          mCallbackUnsupported.report();
        return;
      }
      mSurfaceCreated = true;
      mSurfaceAttached = true;
      mRequireResize = true;
      nativeResumeSurfaceRendering();
      return;
    }

    mRequireResize = false;
    setupWidgets(context, surfaceFrame.width(), surfaceFrame.height());

    final boolean firstStart = mLocationHelper.isInFirstRun();
    if (!nativeCreateEngine(surface, surfaceDpi, firstStart, mLaunchByDeepLink, Config.getVersionCode(),
                            ROMUtils.isCustomROM()))
    {
      if (mCallbackUnsupported != null)
        mCallbackUnsupported.report();
      return;
    }
    sCurrentDpi = surfaceDpi;

    if (firstStart)
      UiThread.runLater(mLocationHelper::onExitFromFirstRun);

    mSurfaceCreated = true;
    mSurfaceAttached = true;
    nativeResumeSurfaceRendering();
    if (mMapRenderingListener != null)
      mMapRenderingListener.onRenderingCreated();
  }

  public void onSurfaceChanged(@NonNull Context context, @NonNull Surface surface, @NonNull Rect surfaceFrame,
                               boolean isSurfaceCreating)
  {
    Logger.d(TAG, "mSurfaceCreated = " + mSurfaceCreated);
    if (!mSurfaceCreated || (!mRequireResize && isSurfaceCreating))
      return;

    nativeSurfaceChanged(surface, surfaceFrame.width(), surfaceFrame.height());

    mRequireResize = false;
    setupWidgets(context, surfaceFrame.width(), surfaceFrame.height());
    nativeApplyWidgets();
    if (mMapRenderingListener != null)
      mMapRenderingListener.onRenderingRestored();
  }

  public void onSurfaceDestroyed(boolean activityIsChangingConfigurations)
  {
    Logger.d(TAG, "mSurfaceCreated = " + mSurfaceCreated + ", mSurfaceAttached = " + mSurfaceAttached);
    if (!mSurfaceCreated || !mSurfaceAttached)
      return;

    nativeDetachSurface(!activityIsChangingConfigurations);
    mSurfaceCreated = !nativeDestroySurfaceOnDetach();
    mSurfaceAttached = false;
  }

  public void setMapRenderingListener(@Nullable MapRenderingListener mapRenderingListener)
  {
    mMapRenderingListener = mapRenderingListener;
  }

  public void setCallbackUnsupported(@Nullable CallbackUnsupported callback)
  {
    mCallbackUnsupported = callback;
  }

  public void onCreate(boolean launchByDeeplink)
  {
    mLaunchByDeepLink = launchByDeeplink;
    mCurrentCompassOffsetX = 0;
    mCurrentCompassOffsetY = 0;
    mBottomWidgetOffsetX = 0;
    mBottomWidgetOffsetY = 0;
  }

  public void onStart()
  {
    nativeSetRenderingInitializationFinishedListener(mMapRenderingListener);
  }

  public void onStop()
  {
    nativeSetRenderingInitializationFinishedListener(null);
  }

  public void onPause()
  {
    // Pause/Resume can be called without surface creation/destroy.
    if (mSurfaceAttached)
      nativePauseSurfaceRendering();
  }

  public void onResume()
  {
    // Pause/Resume can be called without surface creation/destroy.
    if (mSurfaceAttached)
      nativeResumeSurfaceRendering();
  }

  public boolean isContextCreated()
  {
    return mSurfaceCreated;
  }

  public static void onScroll(double distanceX, double distanceY)
  {
    Map.nativeOnScroll(distanceX, distanceY);
  }

  public static void zoomIn()
  {
    nativeScalePlus();
  }

  public static void zoomOut()
  {
    nativeScaleMinus();
  }

  public static void onScale(double factor, double focusX, double focusY, boolean isAnim)
  {
    nativeOnScale(factor, focusX, focusY, isAnim);
  }

  public static void onTouch(@MultiTouchAction int actionType, @NonNull MotionEvent event,
                             @PointerIndex int pointerIndex)
  {
    if (event.getPointerCount() == 1)
    {
      nativeOnTouch(actionType, event.getPointerId(0), event.getX(), event.getY(), PointerIndex.INVALID, 0, 0,
                    PointerIndex.FIRST);
    }
    else
    {
      nativeOnTouch(actionType, event.getPointerId(0), event.getX(0), event.getY(0), event.getPointerId(1),
                    event.getX(1), event.getY(1), pointerIndex);
    }
  }

  public static void onClick(float x, float y)
  {
    nativeOnTouch(MultiTouchAction.DOWN, PointerIndex.FIRST, x, y, PointerIndex.INVALID, 0, 0, PointerIndex.FIRST);
    nativeOnTouch(MultiTouchAction.UP, PointerIndex.FIRST, x, y, PointerIndex.INVALID, 0, 0, PointerIndex.FIRST);
  }

  public static boolean isEngineCreated()
  {
    return nativeIsEngineCreated();
  }

  public static void executeMapApiRequest()
  {
    nativeExecuteMapApiRequest();
  }

  public DisplayType getDisplayType()
  {
    return mDisplayType;
  }

  private void setupWidgets(@NonNull Context context, int width, int height)
  {
    mHeight = height;
    mWidth = width;

    nativeCleanWidgets();
    updateBottomWidgetsOffset(context, mBottomWidgetOffsetX, mBottomWidgetOffsetY);
    if (mDisplayType == DisplayType.Device)
    {
      nativeSetupWidget(WidgetType.SCALE_FPS_LABEL, Utils.dimen(context, R.dimen.margin_base),
                        Utils.dimen(context, R.dimen.margin_base) * 2, Anchor.LEFT_TOP);
      updateCompassOffset(context, mCurrentCompassOffsetX, mCurrentCompassOffsetY, false);
    }
    else
    {
      nativeSetupWidget(WidgetType.SCALE_FPS_LABEL, (float) mWidth / 2 + Utils.dimen(context, R.dimen.margin_base) * 2,
                        Utils.dimen(context, R.dimen.margin_base), Anchor.LEFT_TOP);
      updateCompassOffset(context, mWidth, mCurrentCompassOffsetY, true);
    }
  }

  private void updateRulerOffset(@NonNull Context context, int offsetX, int offsetY)
  {
    // Raise the ruler a bit above the bottom edge so it stays visible over the search bar in compact mode.
    final int additionalOffset = Utils.dimen(context, R.dimen.ruler_additional_offset);
    nativeSetupWidget(WidgetType.RULER, Utils.dimen(context, R.dimen.margin_ruler) + offsetX,
                      mHeight - Utils.dimen(context, R.dimen.margin_ruler) - offsetY - additionalOffset,
                      Anchor.LEFT_BOTTOM);
    if (mSurfaceCreated)
      nativeApplyWidgets();
  }

  private void updateAttributionOffset(@NonNull Context context, int offsetX, int offsetY)
  {
    nativeSetupWidget(WidgetType.COPYRIGHT, Utils.dimen(context, R.dimen.margin_ruler) + offsetX,
                      mHeight - Utils.dimen(context, R.dimen.margin_ruler) - offsetY, Anchor.LEFT_BOTTOM);
    if (mSurfaceCreated)
      nativeApplyWidgets();
  }

  // Engine
  private static native boolean nativeCreateEngine(@NonNull Surface surface, int density, boolean firstLaunch,
                                                   boolean isLaunchByDeepLink, int appVersionCode, boolean isCustomROM);

  private static native boolean nativeIsEngineCreated();

  private static native void nativeUpdateEngineDpi(int dpi);

  private static native void nativeSetRenderingInitializationFinishedListener(@Nullable MapRenderingListener listener);

  private static native void nativeExecuteMapApiRequest();

  // Surface
  private static native boolean nativeAttachSurface(@NonNull Surface surface);

  private static native void nativeDetachSurface(boolean destroySurface);

  private static native void nativeSurfaceChanged(@NonNull Surface surface, int w, int h);

  private static native boolean nativeDestroySurfaceOnDetach();

  private static native void nativePauseSurfaceRendering();

  private static native void nativeResumeSurfaceRendering();

  // Widgets
  private static native void nativeApplyWidgets();

  private static native void nativeCleanWidgets();

  private static native void nativeUpdateMyPositionRoutingOffset(int offsetY);

  private static native void nativeSetupWidget(@WidgetType int widget, float x, float y, @Anchor int anchor);

  private static native void nativeCompassUpdated(double north, boolean forceRedraw);

  // Events
  private static native void nativeScalePlus();

  private static native void nativeScaleMinus();

  private static native void nativeOnScroll(double distanceX, double distanceY);

  private static native void nativeOnScale(double factor, double focusX, double focusY, boolean isAnim);

  private static native void nativeOnTouch(@MultiTouchAction int actionType, @PointerIndex int id1, float x1, float y1,
                                           @PointerIndex int id2, float x2, float y2, @PointerIndex int maskedPointer);
}
