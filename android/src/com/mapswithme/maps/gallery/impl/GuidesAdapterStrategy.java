package com.mapswithme.maps.gallery.impl;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.R;
import com.mapswithme.maps.gallery.Holders;
import com.mapswithme.maps.gallery.ItemSelectedListener;
import com.mapswithme.maps.gallery.RegularAdapterStrategy;
import com.mapswithme.maps.guides.GuidesGallery;

import java.util.List;

public class GuidesAdapterStrategy extends RegularAdapterStrategy<GuidesGallery.Item>
{
  GuidesAdapterStrategy(@NonNull List<GuidesGallery.Item> items,
                        @Nullable ItemSelectedListener<GuidesGallery.Item> listener)
  {
    super(items, null, listener, Integer.MAX_VALUE);
  }

  @NonNull
  @Override
  protected Holders.BaseViewHolder<GuidesGallery.Item> createProductViewHolder(
      @NonNull ViewGroup parent, int viewType)
  {
    View view = LayoutInflater.from(parent.getContext())
                              .inflate(R.layout.guides_discovery_item, parent, false);
    return new Holders.GuideHodler(view, mItems, getListener());
  }

  @NonNull
  @Override
  protected Holders.BaseViewHolder<GuidesGallery.Item> createMoreProductsViewHolder(
      @NonNull ViewGroup parent, int viewType)
  {
    throw new UnsupportedOperationException("Guides adapter doesn't support more item!");
  }
}
