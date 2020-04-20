package com.mapswithme.maps.maplayer;

import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.RecyclerView;
import com.mapswithme.maps.R;
import com.mapswithme.maps.adapter.OnItemClickListener;

import java.util.Objects;

class LayerHolder extends RecyclerView.ViewHolder
{
  @NonNull
  final ImageView mButton;
  @NonNull
  final TextView mTitle;
  @Nullable
  BottomSheetItem mItem;
  @Nullable
  OnItemClickListener<BottomSheetItem> mListener;

  LayerHolder(@NonNull View root)
  {
    super(root);
    mButton = root.findViewById(R.id.btn);
    mTitle = root.findViewById(R.id.name);
    mButton.setOnClickListener(this::onItemClicked);
  }

  @NonNull
  public BottomSheetItem getItem()
  {
    return Objects.requireNonNull(mItem);
  }

  @NonNull
  public OnItemClickListener<BottomSheetItem> getListener()
  {
    return Objects.requireNonNull(mListener);
  }

  private void onItemClicked(@NonNull View v)
  {
    getListener().onItemClick(v, getItem());
  }
}
