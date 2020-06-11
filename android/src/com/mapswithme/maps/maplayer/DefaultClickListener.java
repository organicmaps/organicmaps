package com.mapswithme.maps.maplayer;

import android.view.View;

import androidx.annotation.NonNull;
import com.mapswithme.maps.adapter.OnItemClickListener;
import com.mapswithme.util.SharedPropertiesUtils;

public abstract class DefaultClickListener implements OnItemClickListener<BottomSheetItem>
{
  @NonNull
  private  final LayersAdapter mAdapter;

  public DefaultClickListener(@NonNull LayersAdapter adapter)
  {
    mAdapter = adapter;
  }

  @Override
  public void onItemClick(@NonNull View v, @NonNull BottomSheetItem item)
  {
    Mode mode = item.getMode();
    SharedPropertiesUtils.setLayerMarkerShownForLayerMode(v.getContext(), mode);
    mode.toggle(v.getContext());
    onItemClickInternal(v, item);
    mAdapter.notifyDataSetChanged();
  }

  public abstract void onItemClickInternal(@NonNull View v, @NonNull BottomSheetItem item);
}
