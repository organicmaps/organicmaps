package com.mapswithme.maps.ads;

import android.support.annotation.LayoutRes;

import com.mapswithme.maps.R;

public enum NetworkType
{
  FACEBOOK, GOOGLE, MOPUB, MOPUB_GOOGLE, MYTARGET;

  @LayoutRes
  public int getLayoutId()
  {
    return R.layout.place_page_banner;
  }
}
