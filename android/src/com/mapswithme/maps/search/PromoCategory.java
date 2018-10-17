package com.mapswithme.maps.search;

import android.content.Context;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;

import com.mapswithme.maps.R;

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
          return 4;
        }

        @NonNull
        @Override
        PromoCategoryProcessor createProcessor(@NonNull Context context)
        {
          return new RutaxiPromoProcessor(context);
        }
      };

  @NonNull
  abstract String getKey();

  @StringRes
  abstract int getStringId();

  @NonNull
  abstract String getProvider();

  abstract int getPosition();

  @NonNull
  abstract PromoCategoryProcessor createProcessor(@NonNull Context context);

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
}
