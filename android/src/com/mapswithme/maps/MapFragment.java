package com.mapswithme.maps;

import android.app.Activity;
import android.content.DialogInterface;
import android.graphics.Rect;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.app.AlertDialog;
import android.util.DisplayMetrics;
import android.view.*;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.downloader.DownloadHelper;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.concurrency.UiThread;

public class MapFragment extends BaseMwmFragment
                      implements View.OnTouchListener,
                                 SurfaceHolder.Callback
{
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
  private boolean mEngineCreated;
  private static boolean sWasCopyrightDisplayed;

  public interface MapRenderingListener
  {
    void onRenderingInitialized();
  }

  public static final String FRAGMENT_TAG = MapFragment.class.getName();

  private void setupWidgets(int width, int height)
  {
    mHeight = height;
    mWidth = width;

    nativeCleanWidgets();
    if (!sWasCopyrightDisplayed)
    {
      nativeSetupWidget(WIDGET_COPYRIGHT,
                        mWidth - UiUtils.dimen(R.dimen.margin_ruler_right),
                        mHeight - UiUtils.dimen(R.dimen.margin_ruler_bottom),
                        ANCHOR_RIGHT_BOTTOM);
      sWasCopyrightDisplayed = true;
    }

    nativeSetupWidget(WIDGET_RULER,
                      mWidth - UiUtils.dimen(R.dimen.margin_ruler_right),
                      mHeight - UiUtils.dimen(R.dimen.margin_ruler_bottom),
                      ANCHOR_RIGHT_BOTTOM);

    if (BuildConfig.DEBUG)
    {
      nativeSetupWidget(WIDGET_SCALE_LABEL,
                        UiUtils.dimen(R.dimen.margin_base),
                        UiUtils.dimen(R.dimen.margin_base),
                        ANCHOR_LEFT_TOP);
    }

    setupCompass(0, 0, false);
  }

  void setupCompass(int offsetX, int offsetY, boolean forceRedraw)
  {
    nativeSetupWidget(WIDGET_COMPASS,
                      UiUtils.dimen(R.dimen.margin_compass_left) + offsetX,
                      mHeight - UiUtils.dimen(R.dimen.margin_compass_bottom) + offsetY,
                      ANCHOR_CENTER);
    if (forceRedraw && mEngineCreated)
      nativeApplyWidgets();
  }

  void setupRuler(int offsetX, int offsetY, boolean forceRedraw)
  {
    nativeSetupWidget(WIDGET_RULER,
                      mWidth - UiUtils.dimen(R.dimen.margin_ruler_right) + offsetX,
                      mHeight - UiUtils.dimen(R.dimen.margin_ruler_bottom) + offsetY,
                      ANCHOR_RIGHT_BOTTOM);
    if (forceRedraw && mEngineCreated)
      nativeApplyWidgets();
  }

  private void onRenderingInitialized()
  {
    final Activity activity = getActivity();
    if (isAdded() && activity instanceof MapRenderingListener)
      ((MapRenderingListener) activity).onRenderingInitialized();
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
    final Surface surface = surfaceHolder.getSurface();
    if (nativeIsEngineCreated())
    {
      nativeAttachSurface(surface);
      mRequireResize = true;
      return;
    }

    mRequireResize = false;
    final Rect rect = surfaceHolder.getSurfaceFrame();
    setupWidgets(rect.width(), rect.height());

    final DisplayMetrics metrics = new DisplayMetrics();
    getActivity().getWindowManager().getDefaultDisplay().getMetrics(metrics);
    final float exactDensityDpi = metrics.densityDpi;

    mEngineCreated = nativeCreateEngine(surface, (int) exactDensityDpi);
    if (!mEngineCreated)
    {
      reportUnsupported();
      return;
    }

    onRenderingInitialized();
  }

  @Override
  public void surfaceChanged(SurfaceHolder surfaceHolder, int format, int width, int height)
  {
    if (!mEngineCreated ||
        (!mRequireResize && surfaceHolder.isCreating()))
      return;

    nativeSurfaceChanged(width, height);

    mRequireResize = false;
    setupWidgets(width, height);
    nativeApplyWidgets();
  }

  @Override
  public void surfaceDestroyed(SurfaceHolder surfaceHolder)
  {
    if (!mEngineCreated)
      return;

    if (getActivity() == null || !getActivity().isChangingConfigurations())
    {
      // We're in the main thread here. So nothing from the queue will be run between these two calls.
      // Destroy engine first, then clear the queue that theoretically can be filled by nativeDestroyEngine().
      nativeDestroyEngine();
      MwmApplication.get().clearFunctorsOnUiThread();
    }
    else
      nativeDetachSurface();
  }

  @Override
  public void onCreate(Bundle b)
  {
    super.onCreate(b);
    setRetainInstance(true);
  }

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_map, container, false);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    final SurfaceView surfaceView = (SurfaceView) view.findViewById(R.id.map_surfaceview);
    surfaceView.getHolder().addCallback(this);
    nativeConnectDownloadButton();
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

  @SuppressWarnings("UnusedDeclaration")
  public void onDownloadCountryClicked(final int group, final int country, final int region, final int options)
  {
    UiThread.run(new Runnable()
    {
      @Override
      public void run()
      {
        final MapStorage.Index index = new MapStorage.Index(group, country, region);
        if (options == -1)
        {
          nativeDownloadCountry(index, options);
          return;
        }

        final long size = MapStorage.INSTANCE.countryRemoteSizeInBytes(index, options);
        DownloadHelper.downloadWithCellularCheck(getActivity(), size, MapStorage.INSTANCE.countryName(index), new DownloadHelper.OnDownloadListener()
        {
          @Override
          public void onDownload()
          {
            nativeDownloadCountry(index, options);
          }
        });
      }
    });
  }

  private native void nativeConnectDownloadButton();
  private static native void nativeDownloadCountry(MapStorage.Index index, int options);
  static native void nativeOnLocationError(int errorCode);
  static native void nativeLocationUpdated(long time, double lat, double lon, float accuracy, double altitude, float speed, float bearing);
  static native void nativeCompassUpdated(double magneticNorth, double trueNorth, boolean forceRedraw);
  static native void nativeScalePlus();
  static native void nativeScaleMinus();
  static native boolean nativeShowMapForUrl(String url);
  static native boolean nativeIsEngineCreated();
  private static native boolean nativeCreateEngine(Surface surface, int density);
  private static native void nativeDestroyEngine();
  private static native void nativeAttachSurface(Surface surface);
  private static native void nativeDetachSurface();
  private static native void nativeSurfaceChanged(int w, int h);
  private static native void nativeOnTouch(int actionType, int id1, float x1, float y1, int id2, float x2, float y2, int maskedPointer);
  private static native void nativeSetupWidget(int widget, float x, float y, int anchor);
  private static native void nativeApplyWidgets();
  private static native void nativeCleanWidgets();
}
