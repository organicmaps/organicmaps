package com.mapswithme.maps;

import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Rect;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.util.Config;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.log.Logger;

public class MapFragment extends BaseMwmFragment
                      implements View.OnTouchListener,
                                 SurfaceHolder.Callback
{
  public static final String ARG_LAUNCH_BY_DEEP_LINK = "launch_by_deep_link";
  private static final String TAG = MapFragment.class.getSimpleName();

  // Should correspond to android::MultiTouchAction from Framework.cpp
  private static final int NATIVE_ACTION_UP = 0x01;
  private static final int NATIVE_ACTION_DOWN = 0x02;
  private static final int NATIVE_ACTION_MOVE = 0x03;
  private static final int NATIVE_ACTION_CANCEL = 0x04;

  // Should correspond to gui::EWidget from skin.hpp
  private static final int WIDGET_RULER = 0x01;
  private static final int WIDGET_COMPASS = 0x02;
  private static final int WIDGET_COPYRIGHT = 0x04;
  private static final int WIDGET_SCALE_FPS_LABEL = 0x08;

  // Should correspond to dp::Anchor from drape_global.hpp
  private static final int ANCHOR_CENTER = 0x00;
  private static final int ANCHOR_LEFT = 0x01;
  private static final int ANCHOR_RIGHT = (ANCHOR_LEFT << 1);
  private static final int ANCHOR_TOP = (ANCHOR_RIGHT << 1);
  private static final int ANCHOR_BOTTOM = (ANCHOR_TOP << 1);
  private static final int ANCHOR_LEFT_TOP = (ANCHOR_LEFT | ANCHOR_TOP);
  private static final int ANCHOR_RIGHT_TOP = (ANCHOR_RIGHT | ANCHOR_TOP);
  private static final int ANCHOR_LEFT_BOTTOM = (ANCHOR_LEFT | ANCHOR_BOTTOM);
  private static final int ANCHOR_RIGHT_BOTTOM = (ANCHOR_RIGHT | ANCHOR_BOTTOM);

  // Should correspond to df::TouchEvent::INVALID_MASKED_POINTER from user_event_stream.cpp
  private static final int INVALID_POINTER_MASK = 0xFF;
  private static final int INVALID_TOUCH_ID = -1;

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
  private String mUiThemeOnPause;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private SurfaceView mSurfaceView;
  @Nullable
  private MapRenderingListener mMapRenderingListener;

  private void setupWidgets(int width, int height)
  {
    mHeight = height;
    mWidth = width;
    Context context = requireContext();

    nativeCleanWidgets();
    setupBottomWidgetsOffset(mBottomWidgetOffsetY, mBottomWidgetOffsetX);

    nativeSetupWidget(WIDGET_SCALE_FPS_LABEL,
                      UiUtils.dimen(context, R.dimen.margin_base),
                      UiUtils.dimen(context, R.dimen.margin_base),
                      ANCHOR_LEFT_TOP);

    mCurrentCompassOffsetX = 0;
    mCurrentCompassOffsetY = UiUtils.getCompassYOffset(requireContext());
    setupCompass(mCurrentCompassOffsetY, mCurrentCompassOffsetX, false);
  }

  /**
   * Moves the map compass using the given offsets.
   *
   * @param offsetY Pixel offset from the top. -1 to keep the previous value.
   * @param offsetX Pixel offset from the right.  -1 to keep the previous value.
   * @param forceRedraw True to force the compass to redraw
   */
  void setupCompass(int offsetY, int offsetX, boolean forceRedraw)
  {
    Context context = requireContext();
    int x = offsetX < 0 ? mCurrentCompassOffsetX : offsetX;
    int y = offsetY < 0 ? mCurrentCompassOffsetY : offsetY;
    int navPadding = UiUtils.dimen(context, R.dimen.nav_frame_padding);
    int marginX = UiUtils.dimen(context, R.dimen.margin_compass) + navPadding;
    int marginY = UiUtils.dimen(context, R.dimen.margin_compass_top) + navPadding;
    nativeSetupWidget(WIDGET_COMPASS,
                      mWidth - x - marginX,
                      y + marginY,
                      ANCHOR_CENTER);
    if (forceRedraw && mSurfaceCreated)
      nativeApplyWidgets();
    mCurrentCompassOffsetX = x;
    mCurrentCompassOffsetY = y;
  }

  /**
   * Moves the ruler and copyright using the given offsets.
   *
   * @param offsetY Pixel offset from the bottom. -1 to keep the previous value.
   * @param offsetX Pixel offset from the left.  -1 to keep the previous value.
   */
  void setupBottomWidgetsOffset(int offsetY, int offsetX)
  {
    int x = offsetX < 0 ? mBottomWidgetOffsetX : offsetX;
    int y = offsetY < 0 ? mBottomWidgetOffsetY : offsetY;
    setupRuler(y, x,true);
    setupAttribution(y, x, true);
    mBottomWidgetOffsetX = x;
    mBottomWidgetOffsetY = y;
  }

  private void setupRuler(int offsetY, int offsetX, boolean forceRedraw)
  {
    Context context = requireContext();
    nativeSetupWidget(WIDGET_RULER,
                      UiUtils.dimen(context, R.dimen.margin_ruler) + offsetX,
                      mHeight - UiUtils.dimen(context, R.dimen.margin_ruler) - offsetY,
                      ANCHOR_LEFT_BOTTOM);
    if (forceRedraw && mSurfaceCreated)
      nativeApplyWidgets();
  }

  private void setupAttribution(int offsetY, int offsetX, boolean forceRedraw)
  {
    Context context = requireContext();
    nativeSetupWidget(WIDGET_COPYRIGHT,
                      UiUtils.dimen(context, R.dimen.margin_ruler) + offsetX,
                      mHeight - UiUtils.dimen(context, R.dimen.margin_ruler) - offsetY,
                      ANCHOR_LEFT_BOTTOM);
    if (forceRedraw && mSurfaceCreated)
      nativeApplyWidgets();
  }

  private void reportUnsupported()
  {
    new AlertDialog.Builder(requireActivity())
        .setMessage(getString(R.string.unsupported_phone))
        .setCancelable(false)
        .setPositiveButton(getString(R.string.close), new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            requireActivity().moveTaskToBack(true);
          }
        }).show();
  }

  @Override
  public void surfaceCreated(SurfaceHolder surfaceHolder)
  {
    if (isThemeChangingProcess())
    {
      Logger.d(TAG, "Activity is being recreated due theme changing, skip 'surfaceCreated' callback");
      return;
    }

    Logger.d(TAG, "surfaceCreated, mSurfaceCreated = " + mSurfaceCreated);
    final Surface surface = surfaceHolder.getSurface();
    if (nativeIsEngineCreated())
    {
      if (!nativeAttachSurface(surface))
      {
        reportUnsupported();
        return;
      }
      mSurfaceCreated = true;
      mSurfaceAttached = true;
      mRequireResize = true;
      nativeResumeSurfaceRendering();
      return;
    }

    mRequireResize = false;
    final Rect rect = surfaceHolder.getSurfaceFrame();
    setupWidgets(rect.width(), rect.height());

    final DisplayMetrics metrics = new DisplayMetrics();
    requireActivity().getWindowManager().getDefaultDisplay().getMetrics(metrics);
    final float exactDensityDpi = metrics.densityDpi;

    final boolean firstStart = LocationHelper.INSTANCE.isInFirstRun();
    if (!nativeCreateEngine(surface, (int) exactDensityDpi, firstStart, mLaunchByDeepLink,
                            BuildConfig.VERSION_CODE))
    {
      reportUnsupported();
      return;
    }

    if (firstStart)
    {
      UiThread.runLater(new Runnable()
      {
        @Override
        public void run()
        {
          LocationHelper.INSTANCE.onExitFromFirstRun();
        }
      });
    }

    mSurfaceCreated = true;
    mSurfaceAttached = true;
    nativeResumeSurfaceRendering();
    if (mMapRenderingListener != null)
      mMapRenderingListener.onRenderingCreated();
  }

  @Override
  public void surfaceChanged(SurfaceHolder surfaceHolder, int format, int width, int height)
  {
    if (isThemeChangingProcess())
    {
      Logger.d(TAG, "Activity is being recreated due theme changing, skip 'surfaceChanged' callback");
      return;
    }

    Logger.d(TAG, "surfaceChanged, mSurfaceCreated = " + mSurfaceCreated);
    if (!mSurfaceCreated || (!mRequireResize && surfaceHolder.isCreating()))
      return;

    final Surface surface = surfaceHolder.getSurface();
    nativeSurfaceChanged(surface, width, height);

    mRequireResize = false;
    setupWidgets(width, height);
    nativeApplyWidgets();
    if (mMapRenderingListener != null)
      mMapRenderingListener.onRenderingRestored();
  }

  @Override
  public void surfaceDestroyed(SurfaceHolder surfaceHolder)
  {
    Logger.d(TAG, "surfaceDestroyed");
    destroySurface();
  }

  void destroySurface()
  {
    Logger.d(TAG, "destroySurface, mSurfaceCreated = " + mSurfaceCreated +
             ", mSurfaceAttached = " + mSurfaceAttached + ", isAdded = " + isAdded());
    if (!mSurfaceCreated || !mSurfaceAttached || !isAdded())
      return;

    nativeDetachSurface(!requireActivity().isChangingConfigurations());
    mSurfaceCreated = !nativeDestroySurfaceOnDetach();
    mSurfaceAttached = false;
  }

  @Override
  public void onAttach(Context context)
  {
    super.onAttach(context);
    mMapRenderingListener = (MapRenderingListener) context;
  }

  @Override
  public void onDetach()
  {
    super.onDetach();
    mMapRenderingListener = null;
  }

  @Override
  public void onCreate(Bundle b)
  {
    super.onCreate(b);
    setRetainInstance(true);
    Bundle args = getArguments();
    if (args != null)
      mLaunchByDeepLink = args.getBoolean(ARG_LAUNCH_BY_DEEP_LINK);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    nativeSetRenderingInitializationFinishedListener(mMapRenderingListener);
    Logger.d(TAG, "onStart");
  }

  public void onStop()
  {
    super.onStop();
    nativeSetRenderingInitializationFinishedListener(null);
    Logger.d(TAG, "onStop");
  }

  private boolean isThemeChangingProcess()
  {
    return mUiThemeOnPause != null &&
           !mUiThemeOnPause.equals(Config.getCurrentUiTheme(requireContext()));
  }

  @Override
  public void onPause()
  {
    mUiThemeOnPause = Config.getCurrentUiTheme(requireContext());

    // Pause/Resume can be called without surface creation/destroy.
    if (mSurfaceAttached)
      nativePauseSurfaceRendering();

    super.onPause();
  }

  @Override
  public void onResume()
  {
    super.onResume();

    // Pause/Resume can be called without surface creation/destroy.
    if (mSurfaceAttached)
      nativeResumeSurfaceRendering();
  }

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    View view = inflater.inflate(R.layout.fragment_map, container, false);
    mSurfaceView = view.findViewById(R.id.map_surfaceview);
    mSurfaceView.getHolder().addCallback(this);
    return view;
  }

  @Override
  public boolean onTouch(View view, MotionEvent event)
  {
    final int count = event.getPointerCount();

    if (count == 0)
      return false;

    int action = event.getActionMasked();
    int pointerIndex = event.getActionIndex();
    switch (action)
    {
      case MotionEvent.ACTION_POINTER_UP:
        action = NATIVE_ACTION_UP;
        break;
      case MotionEvent.ACTION_UP:
        action = NATIVE_ACTION_UP;
        pointerIndex = 0;
        break;
      case MotionEvent.ACTION_POINTER_DOWN:
        action = NATIVE_ACTION_DOWN;
        break;
      case MotionEvent.ACTION_DOWN:
        action = NATIVE_ACTION_DOWN;
        pointerIndex = 0;
        break;
      case MotionEvent.ACTION_MOVE:
        action = NATIVE_ACTION_MOVE;
        pointerIndex = INVALID_POINTER_MASK;
        break;
      case MotionEvent.ACTION_CANCEL:
        action = NATIVE_ACTION_CANCEL;
        break;
    }

    switch (count)
    {
      case 1:
        nativeOnTouch(action, event.getPointerId(0), event.getX(), event.getY(), INVALID_TOUCH_ID, 0, 0, 0);
        return true;
      default:
        nativeOnTouch(action,
                      event.getPointerId(0), event.getX(0), event.getY(0),
                      event.getPointerId(1), event.getX(1), event.getY(1), pointerIndex);
        return true;
    }
  }

  boolean isContextCreated()
  {
    return mSurfaceCreated;
  }

  static native void nativeCompassUpdated(double north, boolean forceRedraw);
  static native void nativeScalePlus();
  static native void nativeScaleMinus();
  public static native boolean nativeShowMapForUrl(String url);
  static native boolean nativeIsEngineCreated();
  static native boolean nativeDestroySurfaceOnDetach();
  private static native boolean nativeCreateEngine(Surface surface, int density,
                                                   boolean firstLaunch,
                                                   boolean isLaunchByDeepLink,
                                                   int appVersionCode);
  private static native boolean nativeAttachSurface(Surface surface);
  private static native void nativeDetachSurface(boolean destroySurface);
  private static native void nativePauseSurfaceRendering();
  private static native void nativeResumeSurfaceRendering();
  private static native void nativeSurfaceChanged(Surface surface, int w, int h);
  private static native void nativeOnTouch(int actionType, int id1, float x1, float y1, int id2, float x2, float y2, int maskedPointer);
  private static native void nativeSetupWidget(int widget, float x, float y, int anchor);
  private static native void nativeApplyWidgets();
  private static native void nativeCleanWidgets();
  private static native void nativeSetRenderingInitializationFinishedListener(
      @Nullable MapRenderingListener listener);
}
