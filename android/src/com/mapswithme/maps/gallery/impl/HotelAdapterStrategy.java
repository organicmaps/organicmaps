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
import com.mapswithme.maps.gallery.RegularAdapterStrategy;
import com.mapswithme.maps.search.SearchResult;

import java.util.List;

public class HotelAdapterStrategy extends RegularAdapterStrategy<Items.SearchItem>
{
  HotelAdapterStrategy(@NonNull Context context,
                       @NonNull SearchResult[] results,
                       @Nullable ItemSelectedListener<Items.SearchItem> listener)
  {
    this(SearchBasedAdapterStrategy.convertItems(results), new Items.MoreSearchItem(context), listener);
  }

  private HotelAdapterStrategy(@NonNull List<Items.SearchItem> items,
                               @Nullable Items.SearchItem moreItem,
                               @Nullable ItemSelectedListener<Items.SearchItem> listener)
  {
    super(items, moreItem, listener);
  }

  @NonNull
  @Override
  protected Holders.BaseViewHolder<Items.SearchItem> createProductViewHolder
      (@NonNull ViewGroup parent, int viewType)
  {
    View view = LayoutInflater.from(parent.getContext())
                              .inflate(R.layout.item_discovery_hotel_product, parent, false);
    return new Holders.HotelViewHolder(view, mItems, getListener());
  }

  @NonNull
  @Override
  protected Holders.BaseViewHolder<Items.SearchItem> createMoreProductsViewHolder
      (@NonNull ViewGroup parent, int viewType)
  {
    View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_search_more, parent,
                                                                 false);
    return new Holders.SearchMoreHolder(view, mItems, getListener());
  }
}
