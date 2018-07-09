package com.mapswithme.maps.gallery.impl;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.gallery.GalleryAdapter;
import com.mapswithme.maps.gallery.Holders;
import com.mapswithme.maps.gallery.Items;
import com.mapswithme.maps.gallery.RegularAdapterStrategy;
import com.mapswithme.maps.search.SearchResult;

import java.util.List;

public class HotelAdapterStrategy extends RegularAdapterStrategy<Items.SearchItem>
{
  HotelAdapterStrategy(@NonNull SearchResult[] results)
  {
    this(SearchBasedAdapterStrategy.convertItems(results), new Items.MoreSearchItem());
  }

  private HotelAdapterStrategy(@NonNull List<Items.SearchItem> items, @Nullable Items.SearchItem
      moreItem)
  {
    super(items, moreItem);
  }

  @NonNull
  @Override
  protected Holders.BaseViewHolder<Items.SearchItem> createProductViewHolder
      (@NonNull ViewGroup parent, int viewType,
       @NonNull GalleryAdapter<?, Items.SearchItem> adapter)
  {
    View view = LayoutInflater.from(parent.getContext())
                              .inflate(R.layout.item_discovery_hotel_product, parent, false);
    return new Holders.HotelViewHolder(view, mItems, adapter);
  }

  @NonNull
  @Override
  protected Holders.BaseViewHolder<Items.SearchItem> createMoreProductsViewHolder
      (@NonNull ViewGroup parent, int viewType,
       @NonNull GalleryAdapter<?, Items.SearchItem> adapter)
  {
    View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_search_more, parent,
                                                                 false);
    return new Holders.SearchMoreHolder(view, mItems, adapter);
  }
}
