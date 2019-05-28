package com.mapswithme.maps.gallery.impl;

import android.net.Uri;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;

import com.bumptech.glide.Glide;
import com.mapswithme.maps.R;
import com.mapswithme.maps.gallery.GalleryAdapter;
import com.mapswithme.maps.gallery.Holders;
import com.mapswithme.maps.gallery.ItemSelectedListener;
import com.mapswithme.maps.gallery.RegularAdapterStrategy;

import java.util.List;

class CatalogPromoAdapterStrategy extends RegularAdapterStrategy<RegularAdapterStrategy.Item>
{
  CatalogPromoAdapterStrategy(@NonNull List<Item> items, @Nullable Item moreItem)
  {
    super(items, moreItem);
  }

  @NonNull
  @Override
  protected Holders.BaseViewHolder<Item> createProductViewHolder(@NonNull ViewGroup parent,
                                                                 int viewType,
                                                                 @NonNull GalleryAdapter<?, Item> adapter)
  {
    View view = LayoutInflater.from(parent.getContext())
                              .inflate(R.layout.catalog_promo_item_card, parent,
                                       false);

    return new CatalogPromoHolder(view, mItems, adapter.getListener());
  }

  @NonNull
  @Override
  protected Holders.BaseViewHolder<Item> createMoreProductsViewHolder(@NonNull ViewGroup parent, int viewType,
                                                                      @NonNull GalleryAdapter<?, Item> adapter)
  {
    LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    View view = inflater.inflate(R.layout.item_search_more, parent,false);
    return new Holders.GenericMoreHolder<>(view, mItems, adapter);
  }

  public static class CatalogPromoHolder extends Holders.BaseViewHolder<Item>
  {
    @NonNull
    private final ImageView mImage;

    CatalogPromoHolder(@NonNull View itemView,
                       @NonNull List<Item> items,
                       @Nullable ItemSelectedListener<Item> listener)
    {
      super(itemView, items, listener);
      mImage = itemView.findViewById(R.id.image);
    }

    @Override
    public void bind(@NonNull Item item)
    {
      super.bind(item);
      Glide.with(itemView.getContext())
           .load(Uri.parse(item.getUrl()))
           .placeholder(R.drawable.img_guides_gallery_placeholder)
           .into(mImage);
    }
  }
}
