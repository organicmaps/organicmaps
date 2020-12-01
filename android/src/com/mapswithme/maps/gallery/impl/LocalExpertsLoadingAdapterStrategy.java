package com.mapswithme.maps.gallery.impl;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.gallery.ItemSelectedListener;
import com.mapswithme.maps.gallery.Items;

public class LocalExpertsLoadingAdapterStrategy extends SimpleLoadingAdapterStrategy
{
  LocalExpertsLoadingAdapterStrategy(@NonNull Context context,
                                     @Nullable ItemSelectedListener<Items.Item> listener)
  {
    super(context, listener);
  }

  @NonNull
  @Override
  protected View inflateView(@NonNull LayoutInflater inflater, @NonNull ViewGroup parent)
  {
    return inflater.inflate(R.layout.item_discovery_expert_loading, parent, false);
  }
}
