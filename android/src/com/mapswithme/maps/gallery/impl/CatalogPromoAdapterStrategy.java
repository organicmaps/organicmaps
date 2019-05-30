package com.mapswithme.maps.gallery.impl;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.gallery.Holders;
import com.mapswithme.maps.gallery.ItemSelectedListener;
import com.mapswithme.maps.gallery.RegularAdapterStrategy;

import java.util.List;

class CatalogPromoAdapterStrategy extends RegularAdapterStrategy<RegularAdapterStrategy.Item>
{
  CatalogPromoAdapterStrategy(@NonNull List<Item> items, @Nullable Item moreItem,
                              @Nullable ItemSelectedListener<Item> listener)
  {
    super(items, moreItem, listener);
  }

  @NonNull
  @Override
  protected Holders.BaseViewHolder<Item> createProductViewHolder(@NonNull ViewGroup parent,
                                                                 int viewType)
  {
    View view = LayoutInflater.from(parent.getContext())
                              .inflate(R.layout.catalog_promo_item_card, parent,
                                       false);

    return new Holders.CatalogPromoHolder(view, mItems, getListener());
  }

  @NonNull
  @Override
  protected Holders.BaseViewHolder<Item> createMoreProductsViewHolder(@NonNull ViewGroup parent,
                                                                      int viewType)
  {
    LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    View view = inflater.inflate(R.layout.item_search_more, parent, false);
    return new Holders.GenericMoreHolder<>(view, mItems, getListener());
  }
}
