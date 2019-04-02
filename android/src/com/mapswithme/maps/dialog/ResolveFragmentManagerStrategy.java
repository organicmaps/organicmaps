package com.mapswithme.maps.dialog;

import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;

interface ResolveFragmentManagerStrategy
{
  @NonNull
  FragmentManager resolve(@NonNull Fragment baseFragment);

  @NonNull
  FragmentManager resolve(@NonNull FragmentActivity activity);
}
