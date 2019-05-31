package com.mapswithme.maps.gallery;

import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.view.ViewGroup;

public class GalleryAdapter<VH extends Holders.BaseViewHolder<I>, I extends Items.Item>
    extends RecyclerView.Adapter<VH>
{
  @NonNull
  private final AdapterStrategy<VH, I> mStrategy;

  public GalleryAdapter(@NonNull AdapterStrategy<VH, I> strategy)
  {
    mStrategy = strategy;
  }

  @Override
  public VH onCreateViewHolder(ViewGroup parent, int viewType)
  {
    return mStrategy.createViewHolder(parent, viewType);
  }

  @Override
  public void onBindViewHolder(VH holder, int position)
  {
    mStrategy.onBindViewHolder(holder, position);
  }

  @Override
  public int getItemCount()
  {
    return mStrategy.getItemCount();
  }

  @Override
  public int getItemViewType(int position)
  {
    return mStrategy.getItemViewType(position);
  }
}
