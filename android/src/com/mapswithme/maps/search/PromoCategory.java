package com.mapswithme.maps.search;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;

import java.util.ArrayList;
import java.util.List;

public enum PromoCategory
{
  RUTAXI
      {
        @NonNull
        @Override
        String getKey()
        {
          return "taxi";
        }

        @Override
        int getStringId()
        {
          return R.string.taxi;
        }

        @NonNull
        @Override
        String getProvider()
        {
          return "RuTaxi";
        }

        @Override
        int getPosition()
        {
          return 6;
        }

        @NonNull
        @Override
        PromoCategoryProcessor createProcessor(@NonNull Activity activity)
        {
          return new RutaxiPromoProcessor(activity);
        }

        @Override
        boolean isSupported()
        {
          return Framework.nativeHasRuTaxiCategoryBanner();
        }
      };

  @NonNull
  abstract String getKey();

  @StringRes
  abstract int getStringId();

  @NonNull
  abstract String getProvider();

  abstract int getPosition();

  abstract boolean isSupported();

  @NonNull
  abstract PromoCategoryProcessor createProcessor(@NonNull Activity activity);

  @Nullable
  static PromoCategory findByStringId(@StringRes int nameId)
  {
    for (PromoCategory category : values())
    {
      if (category.getStringId() == nameId)
        return category;
    }
    return null;
  }

  @NonNull
  static List<PromoCategory> supportedValues()
  {
    List<PromoCategory> result = new ArrayList<>();
    for (PromoCategory category : values())
    {
      if (category.isSupported())
        result.add(category);
    }

    return result;
  }
}
