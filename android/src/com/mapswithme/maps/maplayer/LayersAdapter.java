package com.mapswithme.maps.maplayer;

import android.content.Context;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import com.mapswithme.maps.R;
import com.mapswithme.maps.adapter.OnItemClickListener;

import java.util.List;

public class LayersAdapter extends RecyclerView.Adapter<LayerHolder>
{
  @NonNull
  private final List<Pair<BottomSheetItem, OnItemClickListener<BottomSheetItem>>> mItems;

  public LayersAdapter(@NonNull List<Pair<BottomSheetItem, OnItemClickListener<BottomSheetItem>>> modes)
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
    Context context = holder.itemView.getContext();
    Pair<BottomSheetItem, OnItemClickListener<BottomSheetItem>> pair = mItems.get(position);
    BottomSheetItem item = pair.first;
    holder.mItem = item;

    boolean isEnabled = item.getMode().isEnabled(context);

    holder.mButton.setSelected(isEnabled);
    holder.mTitle.setSelected(isEnabled);
    holder.mTitle.setText(item.getTitle());
    holder.mButton.setImageResource(isEnabled ? item.getEnabledStateDrawable()
                                              : item.getDisabledStateDrawable());
    holder.mListener = pair.second;
  }

  @Override
  public int getItemCount()
  {
    return mItems.size();
  }
}
