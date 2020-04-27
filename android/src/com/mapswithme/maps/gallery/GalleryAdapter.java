package com.mapswithme.maps.gallery;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
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

  @NonNull
  @Override
  public VH onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    return mStrategy.createViewHolder(parent, viewType);
  }

  @Override
  public void onBindViewHolder(@NonNull VH holder, int position)
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
