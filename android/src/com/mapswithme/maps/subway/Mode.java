package com.mapswithme.maps.subway;

import android.support.annotation.NonNull;
import android.support.v7.app.AppCompatActivity;

import com.mapswithme.maps.adapter.BottomSheetItem;
import com.mapswithme.maps.traffic.TrafficItem;

public enum Mode
{
  TRAFFIC(new TrafficItem())
      {
        @Override
        public void toggleLayerBtn(@NonNull AppCompatActivity activity, boolean isLayerEnabled)
        {

        }
      },

  SUBWAY(new SubwayItem())
      {
        @Override
        public void toggleLayerBtn(@NonNull AppCompatActivity activity, boolean isLayerEnabled)
        {

        }
      };

  @NonNull
  private final BottomSheetItem mItem;

  Mode(@NonNull BottomSheetItem item)
  {
    mItem = item;
  }

  @NonNull
  public BottomSheetItem getItem()
  {
    return mItem;
  }

  public abstract void toggleLayerBtn(@NonNull AppCompatActivity activity, boolean isLayerEnabled);
}
