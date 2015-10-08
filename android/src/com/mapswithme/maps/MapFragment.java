package com.mapswithme.maps;

import android.app.Activity;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.app.AlertDialog;
import android.util.DisplayMetrics;
import android.view.*;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.downloader.DownloadHelper;

public class MapFragment extends BaseMwmFragment
  implements View.OnTouchListener,
             SurfaceHolder.Callback
{
  // Should be equal to values from Framework.cpp MultiTouchAction enum
  private static final int NATIVE_ACTION_UP = 0x01;
  private static final int NATIVE_ACTION_DOWN = 0x02;
  private static final int NATIVE_ACTION_MOVE = 0x03;
  private static final int NATIVE_ACTION_CANCEL = 0x04;

  // Should correspond to gui::EWidget from skin.hpp
  private static final int WIDGET_RULER = 0x01;
  private static final int WIDGET_COPYRIGHT = 0x02;
  private static final int WIDGET_COUNTRY_STATUS = 0x04;
  private static final int WIDGET_COMPASS = 0x08;

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

  private SurfaceHolder mSurfaceHolder;
  private int mDisplayDensity;

  public interface MapRenderingListener
  {
    void onRenderingInitialized();
  }

  public static final String FRAGMENT_TAG = MapFragment.class.getName();

  @Override
  public void surfaceCreated(SurfaceHolder surfaceHolder)
  {
    mSurfaceHolder = surfaceHolder;
    if (nativeIsEngineCreated())
      nativeAttachSurface(mSurfaceHolder.getSurface());
    else
      initEngine();
  }

  @Override
  public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int w, int h)
  {
    nativeSurfaceResized(w, h);
  }

  @Override
  public void surfaceDestroyed(SurfaceHolder surfaceHolder)
  {
    mSurfaceHolder = null;

    if (getActivity() == null || !getActivity().isChangingConfigurations())
    {
      nativeDestroyEngine();
      MwmApplication.get().clearFunctorsOnUiThread();
    } else
    {
      nativeDetachSurface();
    }
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
  public void onCreate(Bundle b)
  {
    super.onCreate(b);
    final DisplayMetrics metrics = new DisplayMetrics();
    getActivity().getWindowManager().getDefaultDisplay().getMetrics(metrics);
    mDisplayDensity = metrics.densityDpi;
    setRetainInstance(true);
  }

  @Override
  public boolean onTouch(View view, MotionEvent event)
  {
    final int count = event.getPointerCount();

    if (count == 0)
      return false;

    int action = event.getActionMasked();
    int maskedPointer = event.getActionIndex();
    switch (action)
    {
      case MotionEvent.ACTION_POINTER_UP:
        action = NATIVE_ACTION_UP;
        break;
      case MotionEvent.ACTION_UP:
        action = NATIVE_ACTION_UP;
        maskedPointer = 0;
        break;
      case MotionEvent.ACTION_POINTER_DOWN:
        action = NATIVE_ACTION_DOWN;
        break;
      case MotionEvent.ACTION_DOWN:
        action = NATIVE_ACTION_DOWN;
        maskedPointer = 0;
        break;
      case MotionEvent.ACTION_MOVE:
        action = NATIVE_ACTION_MOVE;
        maskedPointer = INVALID_POINTER_MASK;
        break;
      case MotionEvent.ACTION_CANCEL:
        action = NATIVE_ACTION_CANCEL;
        break;
    }

    switch (count)
    {
      case 1:
        final float x = event.getX();
        final float y = event.getY();

        nativeOnTouch(action, event.getPointerId(0), x, y, INVALID_TOUCH_ID, 0, 0, 0);
        return true;

      default:
        final float x0 = event.getX(0);
        final float y0 = event.getY(0);

        final float x1 = event.getX(1);
        final float y1 = event.getY(1);

        nativeOnTouch(action, event.getPointerId(0), x0, y0, event.getPointerId(1), x1, y1, maskedPointer);
        return true;
    }
  }

  private void initEngine()
  {
    if (nativeCreateEngine(mSurfaceHolder.getSurface(), mDisplayDensity))
      onRenderingInitialized();
    else
      reportUnsupported();
  }

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_map, container, false);
  }

  protected void applyWidgetPivots(final int mapHeight, final int mapWidth)
  {
    //Framework.setWidgetPivot(Framework.MAP_WIDGET_RULER,
    //                        mSurfaceWidth - UiUtils.dimen(R.dimen.margin_ruler_right),
    //                         mSurfaceHeight - UiUtils.dimen(R.dimen.margin_ruler_bottom));
    //Framework.setWidgetPivot(Framework.MAP_WIDGET_COPYRIGHT,
    //                         mSurfaceWidth - UiUtils.dimen(R.dimen.margin_ruler_right),
    //                         mSurfaceHeight - UiUtils.dimen(R.dimen.margin_ruler_bottom));

    //adjustCompass(0);
  }

  public void adjustCompass(int offset)
  {
    //Framework.setWidgetPivot(Framework.MAP_WIDGET_COMPASS,
    //                         UiUtils.dimen(R.dimen.margin_compass_left) + offset,
    //                         mSurfaceHeight - UiUtils.dimen(R.dimen.margin_compass_bottom));
  }

  public void onRenderingInitialized()
  {
    final Activity host = getActivity();
    if (isAdded() && host instanceof MapRenderingListener)
    {
      final MapRenderingListener listener = (MapRenderingListener) host;
      listener.onRenderingInitialized();
    }
  }

  public void reportUnsupported()
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

  @SuppressWarnings("UnusedDeclaration")
  public void OnDownloadCountryClicked(final int group, final int country, final int region, final int options)
  {
    getActivity().runOnUiThread(new Runnable()
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

        long size = MapStorage.INSTANCE.countryRemoteSizeInBytes(index, options);
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
  static native void nativeCompassUpdated(double magneticNorth, double trueNorth, boolean force);
  static native void nativeScalePlus();
  static native void nativeScaleMinus();
  static native boolean nativeShowMapForUrl(String url);
  static native boolean nativeIsEngineCreated();
  private static native boolean nativeCreateEngine(Surface surface, int density);
  private static native void nativeDestroyEngine();
  private static native void nativeAttachSurface(Surface surface);
  private static native void nativeDetachSurface();
  private static native void nativeSurfaceResized(int w, int h);
  private static native void nativeOnTouch(int actionType, int id1, float x1, float y1, int id2, float x2, float y2, int maskedPointer);
}
