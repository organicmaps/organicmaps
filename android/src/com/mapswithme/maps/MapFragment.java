package com.mapswithme.maps;

import android.app.Activity;
import android.content.DialogInterface;
import android.content.res.Resources;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.app.AlertDialog;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.downloader.DownloadHelper;
import com.nvidia.devtech.NvEventQueueFragment;

public class MapFragment extends NvEventQueueFragment
{
  public interface MapRenderingListener
  {
    void onRenderingInitialized();
  }

  public static final String FRAGMENT_TAG = MapFragment.class.getSimpleName();

  private boolean mIsRenderingInitialized;

  protected native void nativeStorageConnected();

  protected native void nativeStorageDisconnected();

  protected native void nativeConnectDownloadButton();

  protected native void nativeDownloadCountry(MapStorage.Index index, int options);

  protected native void nativeOnLocationError(int errorCode);

  protected native void nativeLocationUpdated(long time, double lat, double lon, float accuracy, double altitude, float speed, float bearing);

  protected native void nativeCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy);

  protected native void nativeScale(double k);

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
  public void onStop()
  {
    super.onStop();
    mIsRenderingInitialized = false;
  }

  @Override
  protected void applyWidgetPivots(final int mapHeight, final int mapWidth)
  {
    final Resources resources = getResources();
    // TODO need a delay here to make call work
    getView().postDelayed(new Runnable()
    {
      @Override
      public void run()
      {
        Framework.setWidgetPivot(Framework.MAP_WIDGET_RULER, mapWidth - resources.getDimensionPixelOffset(R.dimen.margin_right_ruler), mapHeight - resources.getDimensionPixelOffset(R.dimen.margin_bottom_ruler));
        Framework.setWidgetPivot(Framework.MAP_WIDGET_COMPASS, resources.getDimensionPixelOffset(R.dimen.margin_left_compass), mapHeight - resources.getDimensionPixelOffset(R.dimen.margin_bottom_compass));
      }
    }, 300);
  }

  public boolean isRenderingInitialized()
  {
    return mIsRenderingInitialized;
  }

  @Override
  public void OnRenderingInitialized()
  {
    mIsRenderingInitialized = true;

    final Activity host = getActivity();
    if (host != null && host instanceof MapRenderingListener)
    {
      final MapRenderingListener listener = (MapRenderingListener) host;
      listener.onRenderingInitialized();
    }
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
