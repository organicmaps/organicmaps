package com.mapswithme.maps.gallery.impl;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.gallery.Holders;
import com.mapswithme.maps.gallery.ItemSelectedListener;
import com.mapswithme.maps.gallery.Items;

class CatalogPromoErrorAdapterStrategy extends SimpleErrorAdapterStrategy
{
  CatalogPromoErrorAdapterStrategy(@NonNull Context context, @Nullable ItemSelectedListener<Items.Item> listener)
  {
    super(context, listener);
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
    return new Holders.CatalogErrorHolder(itemView, mItems, getListener());
  }
}
