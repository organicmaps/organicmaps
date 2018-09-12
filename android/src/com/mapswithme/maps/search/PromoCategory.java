package com.mapswithme.maps.search;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.text.TextUtils;

import com.mapswithme.util.UiUtils;

public enum PromoCategory
{
  STUB
      {
        @NonNull
        @Override
        String getKey()
        {
          return "";
        }

        @Override
        int getStringId()
        {
          return UiUtils.NO_ID;
        }

        @NonNull
        @Override
        String getStatisticValue()
        {
          return "";
        }
      };

  @NonNull
  abstract String getKey();

  @StringRes
  abstract int getStringId();

  @NonNull
  abstract String getStatisticValue();

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

  @Nullable
  static PromoCategory findByKey(@Nullable String key)
  {
    if (TextUtils.isEmpty(key))
      return null;

    for (PromoCategory cat : values())
    {
      if (cat.getKey().equals(key))
        return cat;
    }
    return null;
  }
}
