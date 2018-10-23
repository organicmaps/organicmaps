package com.mapswithme.maps.search;

import android.content.Context;
import android.support.annotation.NonNull;

import com.mapswithme.util.Utils;

public class RutaxiPromoProcessor implements PromoCategoryProcessor
{
  @NonNull
  private final Context mContext;

  RutaxiPromoProcessor(@NonNull Context context)
  {
    mContext = context;
  }

  @Override
  public void process()
  {
    Utils.openUrl(mContext, "https://go.onelink.me/2944814706/86db6339");
  }
}
