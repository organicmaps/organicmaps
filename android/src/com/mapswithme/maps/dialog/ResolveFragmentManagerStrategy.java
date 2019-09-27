package com.mapswithme.maps.dialog;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentManager;

public interface ResolveFragmentManagerStrategy
{
  @NonNull
  FragmentManager resolve(@NonNull Fragment baseFragment);

  @NonNull
  FragmentManager resolve(@NonNull FragmentActivity activity);
}
