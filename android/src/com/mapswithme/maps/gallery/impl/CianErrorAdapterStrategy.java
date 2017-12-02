package com.mapswithme.maps.gallery.impl;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.gallery.ErrorAdapterStrategy;

class CianErrorAdapterStrategy extends ErrorAdapterStrategy
{
  CianErrorAdapterStrategy(@Nullable String url)
  {
    super(url);
  }

  @Override
  protected int getTitle()
  {
    return R.string.preloader_cian_title;
  }

  @Override
  protected int getSubtitle()
  {
    return R.string.preloader_cian_message;
  }

  @NonNull
  @Override
  protected View inflateView(@NonNull LayoutInflater inflater, @NonNull ViewGroup parent)
  {
    return inflater.inflate(R.layout.item_cian_loading, parent, false);
  }

  @Override
  protected int getLabelForDetailsView()
  {
    return R.string.preloader_cian_button;
  }
}
