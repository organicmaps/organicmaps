package com.mapswithme.maps.gallery;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.ViewGroup;

import java.util.ArrayList;
import java.util.List;

public abstract class AdapterStrategy<VH extends Holders.BaseViewHolder<I>, I extends Items.Item>
{
  @Nullable
  private final ItemSelectedListener<I> mListener;

  @NonNull
  protected final List<I> mItems = new ArrayList<>();

  AdapterStrategy(@Nullable ItemSelectedListener<I> listener)
  {
    mListener = listener;
  }

  @NonNull
  abstract VH createViewHolder(@NonNull ViewGroup parent, int viewType,
                               @NonNull GalleryAdapter<?, I> adapter);

  protected abstract void onBindViewHolder(Holders.BaseViewHolder<I> holder, int position);

  abstract int getItemViewType(int position);

  public int getItemCount()
  {
    return mItems.size();
  }

  @Nullable
  protected ItemSelectedListener<I> getListener()
  {
    return mListener;
  }
}
