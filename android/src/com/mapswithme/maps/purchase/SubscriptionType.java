package com.mapswithme.maps.purchase;

import androidx.annotation.NonNull;
import com.mapswithme.maps.PrivateVariables;
import com.mapswithme.util.Utils;

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

        @NonNull
        @Override
        String[] getProductIds()
        {
          return Utils.concatArrays(PrivateVariables.adsRemovalNotUsedList(),
                                    PrivateVariables.adsRemovalYearlyProductId(),
                                    PrivateVariables.adsRemovalMonthlyProductId(),
                                    PrivateVariables.adsRemovalWeeklyProductId());
        }

        @NonNull
        @Override
        String getYearlyProductId()
        {
          return PrivateVariables.adsRemovalYearlyProductId();
        }

        @NonNull
        @Override
        String getMonthlyProductId()
        {
          return PrivateVariables.adsRemovalMonthlyProductId();
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

        @NonNull
        @Override
        String[] getProductIds()
        {
          return Utils.concatArrays(PrivateVariables.bookmarksSubscriptionNotUsedList(),
                                    PrivateVariables.bookmarksSubscriptionYearlyProductId(),
                                    PrivateVariables.bookmarksSubscriptionMonthlyProductId());
        }

        @NonNull
        @Override
        String getYearlyProductId()
        {
          return PrivateVariables.bookmarksSubscriptionYearlyProductId();
        }

        @NonNull
        @Override
        String getMonthlyProductId()
        {
          return PrivateVariables.bookmarksSubscriptionMonthlyProductId();
        }
      };

  @NonNull
  abstract String getServerId();

  @NonNull
  abstract String getVendor();

  @NonNull
  abstract String[] getProductIds();

  @NonNull
  abstract String getYearlyProductId();

  @NonNull
  abstract String getMonthlyProductId();
}
