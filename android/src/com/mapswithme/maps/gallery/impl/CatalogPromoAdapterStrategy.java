package com.mapswithme.maps.gallery.impl;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.gallery.Holders;
import com.mapswithme.maps.gallery.ItemSelectedListener;
import com.mapswithme.maps.gallery.RegularAdapterStrategy;
import com.mapswithme.maps.promo.PromoEntity;

import java.util.List;

class CatalogPromoAdapterStrategy extends RegularAdapterStrategy<PromoEntity>
{
  private static final int MAX_ITEMS = 3;

  CatalogPromoAdapterStrategy(@NonNull List<PromoEntity> items, @Nullable PromoEntity moreItem,
                              @Nullable ItemSelectedListener<PromoEntity> listener)
  {
    super(items, moreItem, listener, MAX_ITEMS);
  }

  @NonNull
  @Override
  protected Holders.BaseViewHolder<PromoEntity> createProductViewHolder(@NonNull ViewGroup parent,
                                                                 int viewType)
  {
    View view = LayoutInflater.from(parent.getContext())
                              .inflate(R.layout.catalog_promo_item_card, parent,
                                       false);

    return new Holders.CatalogPromoHolder(view, mItems, getListener());
  }

  @NonNull
  @Override
  protected Holders.BaseViewHolder<PromoEntity> createMoreProductsViewHolder(@NonNull ViewGroup parent,
                                                                      int viewType)
  {
    LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    View view = inflater.inflate(R.layout.item_search_more, parent, false);
    return new Holders.GenericMoreHolder<>(view, mItems, getListener());
  }
}
