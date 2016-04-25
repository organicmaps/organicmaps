package com.mapswithme.maps.routing;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v7.app.AlertDialog;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

import java.util.HashSet;
import java.util.List;
import java.util.Set;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.downloader.CountryItem;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.widget.WheelProgressView;
import com.mapswithme.util.UiUtils;

public class RoutingMapsDownloadFragment extends BaseRoutingErrorDialogFragment
{
  private ViewGroup mItemsFrame;

  private final Set<String> mMaps = new HashSet<>();
  private String[] mMapsArray;

  private int mSubscribeSlot;

  @Override
  void beforeDialogCreated(AlertDialog.Builder builder)
  {
    super.beforeDialogCreated(builder);
    builder.setTitle(R.string.downloading);

    mMapsArray = new String[mMissingMaps.size()];
    for (int i = 0; i < mMissingMaps.size(); i++)
    {
      CountryItem item = mMissingMaps.get(i);
      mMaps.add(item.id);
      mMapsArray[i] = item.id;
    }

    mSubscribeSlot = MapManager.nativeSubscribe(new MapManager.StorageCallback()
    {
      private void update()
      {
        WheelProgressView wheel = getWheel();
        if (wheel != null)
          updateWheel(wheel);
      }

      @Override
      public void onStatusChanged(List<MapManager.StorageCallbackData> data)
      {
        for (MapManager.StorageCallbackData item : data)
          if (mMaps.contains(item.countryId))
          {
            if (item.newStatus == CountryItem.STATUS_DONE)
            {
              mMaps.remove(item.countryId);
              if (mMaps.isEmpty())
              {
                RoutingController.get().checkAndBuildRoute();
                dismissAllowingStateLoss();
                return;
              }
            }

            update();
            return;
          }
      }

      @Override
      public void onProgress(String countryId, long localSize, long remoteSize)
      {
        if (mMaps.contains(countryId))
          update();
      }
    });

    for (String map : mMaps)
      MapManager.nativeDownload(map);
  }

  private View setupFrame(View frame)
  {
    UiUtils.hide(frame.findViewById(R.id.tv__message));
    mItemsFrame = (ViewGroup) frame.findViewById(R.id.items_frame);
    return frame;
  }

  @Override
  View buildSingleMapView(CountryItem map)
  {
    View res = setupFrame(super.buildSingleMapView(map));
    bindGroup(res);
    return res;
  }

  @Override
  View buildMultipleMapView()
  {
    return setupFrame(super.buildMultipleMapView());
  }

  private WheelProgressView getWheel()
  {
    if (mItemsFrame == null)
      return null;

    View frame = mItemsFrame.getChildAt(0);
    WheelProgressView res = (WheelProgressView) frame.findViewById(R.id.progress);
    return ((res != null && UiUtils.isVisible(res)) ? res : null);
  }

  private void updateWheel(WheelProgressView wheel)
  {
    int progress = MapManager.nativeGetOverallProgress(mMapsArray);
    if (progress == 0)
      wheel.setPending(true);
    else
    {
      wheel.setPending(false);
      wheel.setProgress(progress);
    }
  }

  @Override
  void bindGroup(View view)
  {
    WheelProgressView wheel = (WheelProgressView) view.findViewById(R.id.progress);
    UiUtils.show(wheel);
    updateWheel(wheel);
  }

  @Override
  public void onStart()
  {
    super.onStart();

    final AlertDialog dlg = (AlertDialog) getDialog();
    dlg.setCancelable(false);
    Button button = dlg.getButton(AlertDialog.BUTTON_NEGATIVE);
    button.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        for (String item : mMaps)
          MapManager.nativeCancel(item);

        dlg.dismiss();
        RoutingController.get().cancel();
      }
    });
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();

    if (mSubscribeSlot != 0)
    {
      MapManager.nativeUnsubscribe(mSubscribeSlot);
      mSubscribeSlot = 0;
    }
  }

  public static RoutingMapsDownloadFragment create(@Nullable String[] missingMaps)
  {
    Bundle args = new Bundle();
    args.putStringArray(EXTRA_MISSING_MAPS, missingMaps);
    RoutingMapsDownloadFragment res = (RoutingMapsDownloadFragment) Fragment.instantiate(MwmApplication.get(), RoutingMapsDownloadFragment.class.getName());
    res.setArguments(args);
    return res;
  }
}
