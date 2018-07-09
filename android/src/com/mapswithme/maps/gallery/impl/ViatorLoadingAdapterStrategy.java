package com.mapswithme.maps.gallery.impl;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.gallery.LoadingAdapterStrategy;

public class ViatorLoadingAdapterStrategy extends LoadingAdapterStrategy
{
  ViatorLoadingAdapterStrategy(@Nullable String url)
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
    return R.string.preloader_viator_message;
  }

  @NonNull
  @Override
  protected View inflateView(@NonNull LayoutInflater inflater, @NonNull ViewGroup parent)
  {
    return inflater.inflate(R.layout.item_viator_loading, parent, false);
  }

  @Override
  protected int getLabelForDetailsView()
  {
    return R.string.preloader_viator_button;
  }
}
