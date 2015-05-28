package com.mapswithme.maps;

import android.app.Activity;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.app.AlertDialog;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import com.mapswithme.maps.downloader.DownloadHelper;
import com.mapswithme.util.UiUtils;
import com.nvidia.devtech.NvEventQueueFragment;

public class MapFragment extends NvEventQueueFragment
{
  public interface MapRenderingListener
  {
    void onRenderingInitialized();
  }

  public static final String FRAGMENT_TAG = MapFragment.class.getName();

  protected native void nativeConnectDownloadButton();

  protected native void nativeDownloadCountry(MapStorage.Index index, int options);

  protected native void nativeOnLocationError(int errorCode);

  protected native void nativeLocationUpdated(long time, double lat, double lon, float accuracy, double altitude, float speed, float bearing);

  protected native void nativeCompassUpdated(double magneticNorth, double trueNorth, boolean force);

  protected native void nativeScalePlus();
  protected native void nativeScaleMinus();

  public native boolean showMapForUrl(String url);

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
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
    nativeConnectDownloadButton();
  }

  @Override
  protected void applyWidgetPivots()
  {
    Framework.setWidgetPivot(Framework.MAP_WIDGET_RULER,
                             mSurfaceWidth - UiUtils.dimen(R.dimen.margin_ruler_right),
                             mSurfaceHeight - UiUtils.dimen(R.dimen.margin_ruler_bottom));
    Framework.setWidgetPivot(Framework.MAP_WIDGET_COPYRIGHT,
                             mSurfaceWidth - UiUtils.dimen(R.dimen.margin_ruler_right),
                             mSurfaceHeight - UiUtils.dimen(R.dimen.margin_ruler_bottom));

    adjustCompass(0);
  }

  public void adjustCompass(int offset)
  {
    Framework.setWidgetPivot(Framework.MAP_WIDGET_COMPASS,
                             UiUtils.dimen(R.dimen.margin_compass_left) + offset,
                             mSurfaceHeight - UiUtils.dimen(R.dimen.margin_compass_bottom));
  }

  @Override
  public void OnRenderingInitialized()
  {
    final Activity host = getActivity();
    if (isAdded() && host instanceof MapRenderingListener)
    {
      final MapRenderingListener listener = (MapRenderingListener) host;
      listener.onRenderingInitialized();
    }

    super.OnRenderingInitialized();
  }

  @Override
  public void ReportUnsupported()
  {
    getActivity().runOnUiThread(new Runnable()
    {
      @Override
      public void run()
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
                dlg.dismiss();
              }
            })
            .create()
            .show();
      }
    });
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
          nativeDownloadCountry(index, options);
        else
        {
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
      }
    });
  }
}
