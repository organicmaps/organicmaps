package com.mapswithme.maps.gallery;

import android.support.annotation.NonNull;
import android.view.ViewGroup;

import java.util.ArrayList;
import java.util.List;

public abstract class AdapterStrategy<VH extends Holders.BaseViewHolder<I>, I extends Items.Item>
{
  @NonNull
  protected final List<I> mItems = new ArrayList<>();

  @NonNull
  abstract VH createViewHolder(@NonNull ViewGroup parent, int viewType, @NonNull GalleryAdapter adapter);

  protected abstract void onBindViewHolder(Holders.BaseViewHolder<I> holder, int position);

  abstract int getItemViewType(int position);

  public int getItemCount()
  {
    return mItems.size();
  }
}
