package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

import com.mapswithme.maps.PrivateVariables;

public enum SubscriptionType
{
  ADS_REMOVAL
      {
        @NonNull
        @Override
        String getServerId()
        {
          return PrivateVariables.adsRemovalServerId();
        }

        @NonNull
        @Override
        String getVendor()
        {
          return PrivateVariables.adsRemovalVendor();
        }
      },
  BOOKMARKS
      {
        @NonNull
        @Override
        String getServerId()
        {
          return PrivateVariables.bookmarksSubscriptionServerId();
        }

        @NonNull
        @Override
        String getVendor()
        {
          return PrivateVariables.bookmarksSubscriptionVendor();
        }
      };

  @NonNull
  abstract String getServerId();

  @NonNull
  abstract String getVendor();
}
