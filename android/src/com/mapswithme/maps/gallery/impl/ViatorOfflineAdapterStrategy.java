package com.mapswithme.maps.gallery.impl;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.gallery.OfflineAdapterStrategy;

public class ViatorOfflineAdapterStrategy extends OfflineAdapterStrategy
{
  ViatorOfflineAdapterStrategy(@Nullable String url)
  {
    super(url);
  }

  @Override
  protected int getTitle()
  {
    return R.string.preloader_viator_title;
  }

  @Override
  protected int getSubtitle()
  {
    return R.string.common_check_internet_connection_dialog;
  }

  @NonNull
  @Override
  protected View inflateView(@NonNull LayoutInflater inflater, @NonNull ViewGroup parent)
  {
    return inflater.inflate(R.layout.item_viator_loading, parent, false);
  }
}
