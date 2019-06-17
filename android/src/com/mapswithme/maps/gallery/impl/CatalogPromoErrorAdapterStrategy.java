package com.mapswithme.maps.gallery.impl;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.gallery.Holders;
import com.mapswithme.maps.gallery.ItemSelectedListener;
import com.mapswithme.maps.gallery.Items;

import java.util.List;

class CatalogPromoErrorAdapterStrategy extends SimpleErrorAdapterStrategy
{
  CatalogPromoErrorAdapterStrategy(@Nullable ItemSelectedListener<Items.Item> listener)
  {
    super(listener);
  }

  @NonNull
  @Override
  protected View inflateView(@NonNull LayoutInflater inflater, @NonNull ViewGroup parent)
  {
    return inflater.inflate(R.layout.catalog_promo_placeholder_card, parent, false);
  }

  @Override
  protected Holders.SimpleViewHolder createViewHolder(@NonNull View itemView)
  {
    return new ErrorHolder(itemView, mItems, getListener());
  }

  private class ErrorHolder extends Holders.BaseEmptyCatalogHolder
  {

    public ErrorHolder(@NonNull View itemView, @NonNull List<Items.Item> items,
                       @Nullable ItemSelectedListener<Items.Item> listener)
    {
      super(itemView, items, listener);
      View progress = itemView.findViewById(R.id.progress);
      progress.setVisibility(View.INVISIBLE);
      TextView subtitle = itemView.findViewById(R.id.subtitle);
      subtitle.setText("");
    }
  }
}
