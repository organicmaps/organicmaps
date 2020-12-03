package com.mapswithme.maps.search;

import android.app.Activity;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.util.ConnectionState;

import java.util.ArrayList;
import java.util.List;

public enum PromoCategory
{
  MEGAFON
      {
        @Override
        @StringRes
        int getStringId()
        {
          return R.string.megafon;
        }

        @Override
        @DrawableRes
        int getIconId(boolean isNightTheme)
        {
          return isNightTheme ? R.drawable.ic_category_megafon_night : R.drawable.ic_category_megafon;
        }

        @NonNull
        @Override
        String getProvider()
        {
          return "Megafon";
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
          return new MegafonPromoProcessor(activity);
        }

        @Override
        boolean isSupported()
        {
          return Framework.nativeHasMegafonCategoryBanner();
        }

        @Override
        int getCallToActionText()
        {
          return R.string.details;
        }
      },
  CITYMOBIL
      {
        @Override
        int getStringId()
        {
          return R.string.taxi;
        }

        @Override
        @DrawableRes
        int getIconId(boolean isNightTheme)
        {
          return isNightTheme ? R.drawable.ic_logo_citymobil_category_night
                              : R.drawable.ic_logo_citymobil_category;
        }

        @NonNull
        @Override
        String getProvider()
        {
          return "Citymobil";
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
          return new CitymobilPromoProcessor(activity);
        }

        @Override
        boolean isSupported()
        {
          return ConnectionState.INSTANCE.isConnected() && Framework.nativeHasCitymobilCategoryBanner();
        }

        @Override
        int getCallToActionText()
        {
          return R.string.taxi_category_order;
        }
      };

  @StringRes
  abstract int getStringId();

  @DrawableRes
  abstract int getIconId(boolean isNightTheme);

  @NonNull
  abstract String getProvider();

  abstract int getPosition();

  abstract boolean isSupported();

  @StringRes
  abstract int getCallToActionText();

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
