package com.mapswithme.maps.gallery.impl;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.discovery.LocalExpert;
import com.mapswithme.maps.gallery.Holders;
import com.mapswithme.maps.gallery.ItemSelectedListener;
import com.mapswithme.maps.gallery.Items;
import com.mapswithme.maps.gallery.RegularAdapterStrategy;

import java.util.ArrayList;
import java.util.List;

import static com.mapswithme.maps.gallery.Constants.TYPE_PRODUCT;

public class LocalExpertsAdapterStrategy extends RegularAdapterStrategy<Items.LocalExpertItem>
{
  LocalExpertsAdapterStrategy(@NonNull LocalExpert[] experts, @Nullable String moreUrl,
                              @Nullable ItemSelectedListener<Items.LocalExpertItem> listener)
  {
    super(convertItems(experts), new Items.LocalExpertMoreItem(moreUrl), listener);
  }

  @NonNull
  @Override
  protected Holders.BaseViewHolder<Items.LocalExpertItem> createProductViewHolder
      (@NonNull ViewGroup parent, int viewType)
  {
    View view = LayoutInflater.from(parent.getContext())
                              .inflate(R.layout.item_discovery_expert, parent,
                                       false);
    return new Holders.LocalExpertViewHolder(view, mItems, getListener());
  }

  @NonNull
  @Override
  protected Holders.BaseViewHolder<Items.LocalExpertItem> createMoreProductsViewHolder
      (@NonNull ViewGroup parent, int viewType)
  {
    View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_viator_more, parent,
                                                                 false);
    return new Holders.GenericMoreHolder<>(view, mItems, getListener());
  }

  @NonNull
  private static List<Items.LocalExpertItem> convertItems(@NonNull LocalExpert[] items)
  {
    List<Items.LocalExpertItem> viewItems = new ArrayList<>();
    for (LocalExpert expert : items)
    {
      viewItems.add(new Items.LocalExpertItem(TYPE_PRODUCT, expert.getName(), expert.getPageUrl(),
                                              expert.getPhotoUrl(), expert.getPrice(),
                                              expert.getCurrency(), expert.getRating()));
    }

    return viewItems;
  }
}
