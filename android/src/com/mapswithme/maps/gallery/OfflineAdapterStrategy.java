package com.mapswithme.maps.gallery;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.view.View;

import com.mapswithme.maps.R;

public abstract class OfflineAdapterStrategy
    extends SingleItemAdapterStrategy<Holders.OfflineViewHolder>
{
  protected OfflineAdapterStrategy(@Nullable String url,
                                   @Nullable ItemSelectedListener<Items.Item> listener)
  {
    super(url, listener);
  }

  @Override
  protected Holders.OfflineViewHolder createViewHolder(@NonNull View itemView)
  {
    return new Holders.OfflineViewHolder(itemView, mItems, getListener());
  }

  @Override
  protected int getLabelForDetailsView()
  {
    return R.string.details;
  }
}
