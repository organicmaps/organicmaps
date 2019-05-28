package com.mapswithme.maps.gallery.impl;

import android.support.annotation.NonNull;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.gallery.GalleryAdapter;
import com.mapswithme.maps.gallery.Holders;
import com.mapswithme.maps.gallery.Items;

import java.util.List;

class CatalogPromoLoadingAdapterStrategy extends SimpleLoadingAdapterStrategy
{
  @NonNull
  @Override
  protected View inflateView(@NonNull LayoutInflater inflater, @NonNull ViewGroup parent)
  {
    return inflater.inflate(R.layout.catalog_promo_placeholder_card, parent, false);
  }

  @Override
  protected Holders.SimpleViewHolder createViewHolder(@NonNull View itemView, @NonNull GalleryAdapter<?, Items.Item> adapter)
  {
    return new CrossPromoLoadingHolder(itemView, mItems, adapter);
  }

  private class CrossPromoLoadingHolder extends Holders.SimpleViewHolder
  {
    CrossPromoLoadingHolder(@NonNull View itemView, @NonNull List<Items.Item> items,
                            @NonNull GalleryAdapter<?, Items.Item> adapter)
    {
      super(itemView, items, adapter);
      TextView subtitle = itemView.findViewById(R.id.subtitle);
      subtitle.setText("");
    }
  }
}
