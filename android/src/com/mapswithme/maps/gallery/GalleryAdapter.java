package com.mapswithme.maps.gallery;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.view.ViewGroup;

public class GalleryAdapter<VH extends Holders.BaseViewHolder<I>, I extends Items.Item>
    extends RecyclerView.Adapter<VH>
{
  @NonNull
  private final AdapterStrategy<VH, I> mStrategy;
  @Nullable
  private ItemSelectedListener mListener;
  @Nullable
  public ItemSelectedListener getListener()
  {
    return mListener;
  }

  public GalleryAdapter(@NonNull AdapterStrategy<VH, I> strategy,
                        @Nullable ItemSelectedListener listener)
  {
    mStrategy = strategy;
    mListener = listener;
  }

  @Override
  public VH onCreateViewHolder(ViewGroup parent, int viewType)
  {
    return mStrategy.createViewHolder(parent, viewType, this);
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

  public interface ItemSelectedListener
  {
    void onItemSelected(@NonNull String url);
    void onMoreItemSelected(@NonNull String url);
  }
}
