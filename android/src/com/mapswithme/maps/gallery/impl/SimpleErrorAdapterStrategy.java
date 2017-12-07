package com.mapswithme.maps.gallery.impl;

import android.support.annotation.NonNull;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.gallery.GalleryAdapter;
import com.mapswithme.maps.gallery.Holders;
import com.mapswithme.maps.gallery.Items;
import com.mapswithme.maps.gallery.SimpleSingleItemAdapterStrategy;

public class SimpleErrorAdapterStrategy
    extends SimpleSingleItemAdapterStrategy<Holders.SimpleViewHolder>
{
  @Override
  protected int getTitle()
  {
    return R.string.discovery_button_other_error_message;
  }

  @NonNull
  @Override
  protected View inflateView(@NonNull LayoutInflater inflater, @NonNull ViewGroup parent)
  {
    return inflater.inflate(R.layout.item_discovery_simple_error, parent, false);
  }

  @Override
  protected Holders.SimpleViewHolder createViewHolder(@NonNull View itemView,
                                                      @NonNull GalleryAdapter<?, Items.Item>
                                                          adapter)
  {
    return new Holders.SimpleViewHolder(itemView, mItems, adapter);
  }
}
