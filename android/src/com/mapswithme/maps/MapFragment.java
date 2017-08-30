package com.mapswithme.maps;

import android.app.Activity;
import android.content.DialogInterface;
import android.graphics.Rect;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.app.AlertDialog;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.util.Config;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

public class MapFragment extends BaseMwmFragment
                      implements View.OnTouchListener,
                                 SurfaceHolder.Callback
{
  public static final String ARG_LAUNCH_BY_DEEP_LINK = "launch_by_deep_link";
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
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
  private static final int WIDGET_SCALE_LABEL = 0x08;

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

  private int mHeight;
  private int mWidth;
  private boolean mRequireResize;
  private boolean mContextCreated;
  private boolean mLaunchByDeepLink;
  private static boolean sWasCopyrightDisplayed;
  @Nullable
  private String mUiThemeOnPause;
  @NonNull
  private SurfaceView mSurfaceView;

  interface MapRenderingListener
  {
    void onRenderingInitialized();
    void onRenderingRestored();
  }

  private void setupWidgets(int width, int height)
  {
    mHeight = height;
    mWidth = width;

    nativeCleanWidgets();
    if (!sWasCopyrightDisplayed)
    {
      nativeSetupWidget(WIDGET_COPYRIGHT,
                        UiUtils.dimen(R.dimen.margin_ruler_left),
                        mHeight - UiUtils.dimen(R.dimen.margin_ruler_bottom),
                        ANCHOR_LEFT_BOTTOM);
      sWasCopyrightDisplayed = true;
    }

    nativeSetupWidget(WIDGET_RULER,
                      UiUtils.dimen(R.dimen.margin_ruler_left),
                      mHeight - UiUtils.dimen(R.dimen.margin_ruler_bottom),
                      ANCHOR_LEFT_BOTTOM);

    if (BuildConfig.DEBUG)
    {
      nativeSetupWidget(WIDGET_SCALE_LABEL,
                        UiUtils.dimen(R.dimen.margin_base),
                        UiUtils.dimen(R.dimen.margin_base),
                        ANCHOR_LEFT_TOP);
    }

    setupCompass(UiUtils.getCompassYOffset(getContext()), false);
  }

  void setupCompass(int offsetY, boolean forceRedraw)
  {
    int navPadding = UiUtils.dimen(R.dimen.nav_frame_padding);
    int marginX = UiUtils.dimen(R.dimen.margin_compass) + navPadding;
    int marginY = UiUtils.dimen(R.dimen.margin_compass_top) + navPadding;
    nativeSetupWidget(WIDGET_COMPASS,
                      mWidth - marginX,
                      offsetY + marginY,
                      ANCHOR_CENTER);
    if (forceRedraw && mContextCreated)
      nativeApplyWidgets();
  }

  void setupRuler(int offsetX, int offsetY, boolean forceRedraw)
  {
    nativeSetupWidget(WIDGET_RULER,
                      UiUtils.dimen(R.dimen.margin_ruler_left) + offsetX,
                      mHeight - UiUtils.dimen(R.dimen.margin_ruler_bottom) + offsetY,
                      ANCHOR_LEFT_BOTTOM);
    if (forceRedraw && mContextCreated)
      nativeApplyWidgets();
  }

  private void onRenderingInitialized()
  {
    final Activity activity = getActivity();
    if (isAdded() && activity instanceof MapRenderingListener)
      ((MapRenderingListener) activity).onRenderingInitialized();
  }

  private void onRenderingRestored()
  {
    final Activity activity = getActivity();
    if (isAdded() && activity instanceof MapRenderingListener)
      ((MapRenderingListener) activity).onRenderingRestored();
  }

  private void reportUnsupported()
  {
    new AlertDialog.Builder(getActivity())
        .setMessage(getString(R.string.unsupported_phone))
        .setCancelable(false)
        .setPositiveButton(getString(R.string.close), new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            getActivity().moveTaskToBack(true);
          }
        }).show();
  }

  @Override
  public void surfaceCreated(SurfaceHolder surfaceHolder)
  {
    LOGGER.d(TAG, "surfaceCreated, mContextCreated = " + mContextCreated);
    final Surface surface = surfaceHolder.getSurface();
    if (nativeIsEngineCreated())
    {
      if (!nativeAttachSurface(surface))
      {
        reportUnsupported();
        return;
      }
      mContextCreated = true;
      mRequireResize = true;
      return;
    }

    mRequireResize = false;
    final Rect rect = surfaceHolder.getSurfaceFrame();
    setupWidgets(rect.width(), rect.height());

    final DisplayMetrics metrics = new DisplayMetrics();
    getActivity().getWindowManager().getDefaultDisplay().getMetrics(metrics);
    final float exactDensityDpi = metrics.densityDpi;

    final boolean firstStart = SplashActivity.isFirstStart();
    if (!nativeCreateEngine(surface, (int) exactDensityDpi, firstStart, mLaunchByDeepLink))
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

    mContextCreated = true;
    onRenderingInitialized();
  }

  @Override
  public void surfaceChanged(SurfaceHolder surfaceHolder, int format, int width, int height)
  {
    LOGGER.d(TAG, "surfaceChanged, mContextCreated = " + mContextCreated);
    if (!mContextCreated ||
        (!mRequireResize && surfaceHolder.isCreating()))
      return;

    nativeSurfaceChanged(width, height);

    mRequireResize = false;
    setupWidgets(width, height);
    nativeApplyWidgets();
    onRenderingRestored();
  }

  @Override
  public void surfaceDestroyed(SurfaceHolder surfaceHolder)
  {
    LOGGER.d(TAG, "surfaceDestroyed");
    destroyContext();
  }

  void destroyContext()
  {
    LOGGER.d(TAG, "destroyContext, mContextCreated = " + mContextCreated +
                  ", isAdded = " + isAdded(), new Throwable());
    if (!mContextCreated || !isAdded())
      return;

    mSurfaceView.getHolder().removeCallback(this);
    nativeDetachSurface(!getActivity().isChangingConfigurations());
    mContextCreated = false;
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
    if (isGoingToBeRecreated())
      return;

    LOGGER.d(TAG, "onStart, surface.addCallback");
    mSurfaceView.getHolder().addCallback(this);
  }

  private boolean isGoingToBeRecreated()
  {
    return mUiThemeOnPause != null && !mUiThemeOnPause.equals(Config.getCurrentUiTheme());
  }

  @Override
  public void onPause()
  {
    mUiThemeOnPause = Config.getCurrentUiTheme();
    super.onPause();
  }

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    View view = inflater.inflate(R.layout.fragment_map, container, false);
    mSurfaceView = (SurfaceView) view.findViewById(R.id.map_surfaceview);
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
    return mContextCreated;
  }

  static native void nativeCompassUpdated(double magneticNorth, double trueNorth, boolean forceRedraw);
  static native void nativeScalePlus();
  static native void nativeScaleMinus();
  static native boolean nativeShowMapForUrl(String url);
  static native boolean nativeIsEngineCreated();
  private static native boolean nativeCreateEngine(Surface surface, int density,
                                                   boolean firstLaunch,
                                                   boolean isLaunchByDeepLink);
  private static native boolean nativeAttachSurface(Surface surface);
  private static native void nativeDetachSurface(boolean destroyContext);
  private static native void nativeSurfaceChanged(int w, int h);
  private static native void nativeOnTouch(int actionType, int id1, float x1, float y1, int id2, float x2, float y2, int maskedPointer);
  private static native void nativeSetupWidget(int widget, float x, float y, int anchor);
  private static native void nativeApplyWidgets();
  private static native void nativeCleanWidgets();
}
