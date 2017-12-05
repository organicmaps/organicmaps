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

import java.util.ArrayList;
import java.util.List;

class SearchBasedAdapterStrategy extends RegularAdapterStrategy<Items.SearchItem>
{
  SearchBasedAdapterStrategy(@NonNull SearchResult[] results)
  {
    this(convertItems(results), null);
  }

  private SearchBasedAdapterStrategy(@NonNull List<Items.SearchItem> items,
                                     @Nullable Items.SearchItem moreItem)
  {
    super(items, moreItem);
  }

  @NonNull
  @Override
  protected Holders.BaseViewHolder<Items.SearchItem> createProductViewHodler
      (@NonNull ViewGroup parent, int viewType, @NonNull GalleryAdapter<?, Items.SearchItem> adapter)
  {
    View view = LayoutInflater.from(parent.getContext())
                              .inflate(R.layout.item_discovery_search, parent, false);
    return new Holders.SearchViewHolder(view, mItems, adapter);
  }

  @NonNull
  @Override
  protected final Holders.BaseViewHolder<Items.SearchItem> createMoreProductsViewHolder
      (@NonNull ViewGroup parent, int viewType, @NonNull GalleryAdapter<?, Items.SearchItem> adapter)
  {
    throw new UnsupportedOperationException("More item is not supported yet for this strategy!");
  }

  @NonNull
  private static List<Items.SearchItem> convertItems(@NonNull SearchResult[] results)
  {
    List<Items.SearchItem> viewItems = new ArrayList<>();
    for (SearchResult result : results)
      viewItems.add(new Items.SearchItem(result));
    return viewItems;
  }
}
