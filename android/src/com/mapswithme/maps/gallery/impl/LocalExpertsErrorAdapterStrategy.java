package com.mapswithme.maps.gallery.impl;

import android.support.annotation.NonNull;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;

public class LocalExpertsErrorAdapterStrategy extends SimpleErrorAdapterStrategy
{
  @NonNull
  @Override
  protected View inflateView(@NonNull LayoutInflater inflater, @NonNull ViewGroup parent)
  {
    return inflater.inflate(R.layout.item_discovery_expert_error, parent, false);
  }
}
