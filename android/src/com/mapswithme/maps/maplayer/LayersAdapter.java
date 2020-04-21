package com.mapswithme.maps.maplayer;

import android.content.Context;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.RecyclerView;
import com.mapswithme.maps.R;
import com.mapswithme.maps.adapter.OnItemClickListener;
import com.mapswithme.util.SharedPropertiesUtils;
import com.mapswithme.util.UiUtils;

import java.util.List;
import java.util.Objects;

public class LayersAdapter extends RecyclerView.Adapter<LayerHolder>
{
  @Nullable
  private List<Pair<BottomSheetItem, OnItemClickListener<BottomSheetItem>>> mItems;

  public LayersAdapter()
  {
  }

  public void setLayerModes(@NonNull List<Pair<BottomSheetItem, OnItemClickListener<BottomSheetItem>>> modes)
  {
    mItems = modes;
  }

  @Override
  public LayerHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    View root = inflater.inflate(R.layout.item_bottomsheet_dialog, parent, false);
    return new LayerHolder(root);
  }

  @Override
  public void onBindViewHolder(LayerHolder holder, int position)
  {
    Objects.requireNonNull(mItems);
    Context context = holder.itemView.getContext();
    Pair<BottomSheetItem, OnItemClickListener<BottomSheetItem>> pair = mItems.get(position);
    BottomSheetItem item = pair.first;
    holder.mItem = item;

    boolean isEnabled = item.getMode().isEnabled(context);

    holder.mButton.setSelected(isEnabled);
    holder.mTitle.setSelected(isEnabled);
    holder.mTitle.setText(item.getTitle());
    boolean isNewLayer = SharedPropertiesUtils.shouldShowNewMarkerForLayerMode(context,
                                                                               item.getMode());
    UiUtils.showIf(isNewLayer, holder.mNewMarker);
    holder.mButton.setImageResource(isEnabled ? item.getEnabledStateDrawable()
                                              : item.getDisabledStateDrawable());
    holder.mListener = pair.second;
  }

  @Override
  public int getItemCount()
  {
    Objects.requireNonNull(mItems);
    return mItems.size();
  }
}
