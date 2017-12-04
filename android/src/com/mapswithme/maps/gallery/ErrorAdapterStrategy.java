package com.mapswithme.maps.gallery;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.View;

public abstract class ErrorAdapterStrategy extends SingleItemAdapterStrategy<Holders.ErrorViewHolder>
{
  protected ErrorAdapterStrategy(@Nullable String url)
  {
    super(url);
  }

  @Override
  protected Holders.ErrorViewHolder createViewHolder(@NonNull View itemView, @NonNull GalleryAdapter adapter)
  {
    return new Holders.ErrorViewHolder(itemView, mItems, adapter);
  }
}
