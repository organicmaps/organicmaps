package com.mapswithme.maps.discovery;

import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Arrays;

/**
 * Represents discovery::ClientParams from core.
 */
public final class DiscoveryParams {
    public static final int ITEM_TYPE_VIATOR = 0;
    public static final int ITEM_TYPE_ATTRACTIONS = 1;
    public static final int ITEM_TYPE_CAFES = 2;
    public static final int ITEM_TYPE_HOTELS = 3;
    public static final int ITEM_TYPE_LOCAL_EXPERTS = 4;

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({ ITEM_TYPE_VIATOR, ITEM_TYPE_ATTRACTIONS, ITEM_TYPE_CAFES, ITEM_TYPE_HOTELS, ITEM_TYPE_LOCAL_EXPERTS })

    @interface ItemType {}

    @Nullable
    private final String mCurrency;
    @Nullable
    private final String mLang;
    private final int mItemsCount;
    @NonNull
    private final int[] mItemTypes;

    public DiscoveryParams(@Nullable String currency, @Nullable String lang, int itemsCount, @NonNull int[] itemTypes)
    {
      mCurrency = currency;
      mLang = lang;
      mItemsCount = itemsCount;
      mItemTypes = itemTypes;
    }

    @Nullable
    public String getCurrency() { return mCurrency; }

    @Nullable
    public String getLang() { return mLang; }

    public int getItemsCount() { return mItemsCount; }

    @NonNull
    public int[] getItemTypes() { return mItemTypes; }

    @Override
    public String toString()
    {
        return "DiscoveryParams{" +
               "mCurrency='" + mCurrency + '\'' +
               ", mLang='" + mLang + '\'' +
               ", mItemsCount=" + mItemsCount +
               ", mItemTypes=" + Arrays.toString(mItemTypes) +
               '}';
    }
}
