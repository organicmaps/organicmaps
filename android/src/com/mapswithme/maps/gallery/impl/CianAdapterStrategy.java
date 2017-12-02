package com.mapswithme.maps.gallery.impl;

import android.content.Context;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.cian.RentOffer;
import com.mapswithme.maps.cian.RentPlace;
import com.mapswithme.maps.gallery.GalleryAdapter;
import com.mapswithme.maps.gallery.Holders;
import com.mapswithme.maps.gallery.Items;
import com.mapswithme.maps.gallery.RegularAdapterStrategy;

import java.util.ArrayList;
import java.util.List;

class CianAdapterStrategy extends RegularAdapterStrategy<Items.CianItem>
{

  CianAdapterStrategy(@NonNull RentPlace[] items, @Nullable String moreUrl)
  {
    super(convertItems(items), new Items.CianMoreItem(moreUrl));
  }

  CianAdapterStrategy(@NonNull List<Items.CianItem> items,
                      @Nullable Items.CianItem moreItem)
  {
    super(items, moreItem);
  }

  @NonNull
  @Override
  protected Holders.BaseViewHolder<Items.CianItem> createProductViewHodler(@NonNull ViewGroup parent,
                                                                           int viewType,
                                                                           @NonNull GalleryAdapter adapter)
  {
    View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_cian_product, parent,
                                                                 false);
    return new Holders.CianProductViewHolder(view, mItems, adapter);
  }

  @NonNull
  @Override
  protected Holders.BaseViewHolder<Items.CianItem> createMoreProductsViewHolder(@NonNull ViewGroup parent,
                                                                                int viewType,
                                                                                @NonNull GalleryAdapter adapter)
  {
    View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_cian_more, parent,
                                                                 false);
    return new Holders.CianMoreItemViewHolder<>(view, mItems, adapter);
  }

  @Override
  protected void onBindViewHolder(Holders.BaseViewHolder<Items.CianItem> holder, int position)
  {
    holder.bind(mItems.get(position));
  }

  @NonNull
  private static List<Items.CianItem> convertItems(@NonNull RentPlace[] items)
  {
    List<Items.CianItem> viewItems = new ArrayList<>();
    for (RentPlace place : items)
    {
      if (place.getOffers().isEmpty())
        continue;

      RentOffer product = place.getOffers().get(0);
      Context context = MwmApplication.get();
      String title = context.getString(R.string.room, Integer.toString(product.getRoomsCount()));
      String price = Integer.toString((int) product.getPrice()) + " "
                     + context.getString(R.string.rub_month);
      viewItems.add(new Items.CianItem(title, product.getUrl(), price, product.getAddress()));
    }
    return viewItems;
  }
}
