package com.mapswithme.maps.gallery;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.View;

public abstract class LoadingAdapterStrategy
    extends SingleItemAdapterStrategy<Holders.LoadingViewHolder>
{

  protected LoadingAdapterStrategy(@Nullable String url, @Nullable ItemSelectedListener<Items.Item> listener)
  {
    super(url, listener);
  }

  @Override
  protected Holders.LoadingViewHolder createViewHolder(@NonNull View itemView)
  {
    return new Holders.LoadingViewHolder(itemView, mItems, getListener());
  }
}
