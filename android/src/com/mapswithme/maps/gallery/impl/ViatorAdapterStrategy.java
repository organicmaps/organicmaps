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
import com.mapswithme.maps.viator.ViatorProduct;

import java.util.ArrayList;
import java.util.List;

public class  ViatorAdapterStrategy
    extends RegularAdapterStrategy<Items.ViatorItem>
{
  ViatorAdapterStrategy(@NonNull ViatorProduct[] products, @Nullable String moreUrl)
  {
    super(convertItems(products), new Items.ViatorMoreItem(moreUrl));
  }

  @NonNull
  @Override
  protected Holders.BaseViewHolder<Items.ViatorItem> createProductViewHodler
      (@NonNull ViewGroup parent, int viewType, @NonNull GalleryAdapter<?, Items.ViatorItem> adapter)
  {
    View view = LayoutInflater.from(parent.getContext())
                              .inflate(R.layout.item_viator_product, parent,
                                       false);
    return new Holders.ViatorProductViewHolder(view, mItems, adapter);
  }

  @NonNull
  @Override
  protected Holders.BaseViewHolder<Items.ViatorItem> createMoreProductsViewHolder
      (@NonNull ViewGroup parent, int viewType, @NonNull GalleryAdapter<?, Items.ViatorItem> adapter)
  {
    View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_viator_more, parent,
                                                                 false);
    return new Holders.ViatorMoreItemViewHolder(view, mItems, adapter);
  }

  @NonNull
  private static List<Items.ViatorItem> convertItems(@NonNull ViatorProduct[] items)
  {
    List<Items.ViatorItem> viewItems = new ArrayList<>();
    for (ViatorProduct product : items)
    {
      viewItems.add(new Items.ViatorItem(product.getPhotoUrl(), product.getTitle(),
                                         product.getDuration(), product.getRating(), product.getPriceFormatted(),
                                         product.getPageUrl()));
    }

    return viewItems;
  }
}
