package com.mapswithme.maps.gallery;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.View;

import com.mapswithme.maps.R;

public abstract class OfflineAdapterStrategy extends SingleItemAdapterStrategy<Holders
    .OfflineViewHolder>
{
  protected OfflineAdapterStrategy(@Nullable String url)
  {
    super(url);
  }

  @Override
  protected Holders.OfflineViewHolder createViewHolder(@NonNull View itemView, @NonNull
      GalleryAdapter<?, Items.Item> adapter)
  {
    return new Holders.OfflineViewHolder(itemView, mItems, adapter);
  }

  @Override
  protected int getLabelForDetailsView()
  {
    return R.string.details;
  }
}
