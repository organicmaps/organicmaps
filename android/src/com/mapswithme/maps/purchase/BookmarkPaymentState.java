package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

enum BookmarkPaymentState
{
  NONE
      {
        @Override
        void activate(@NonNull BookmarkPaymentFragment fragment)
        {
          throw new UnsupportedOperationException("This state can't be used!");
        }
      },
  TRANSACTION_STARTING
      {
        @Override
        void activate(@NonNull BookmarkPaymentFragment fragment)
        {
          // TODO: coming soon.
        }
      },
  TRANSACTION_FAILURE
      {
        @Override
        void activate(@NonNull BookmarkPaymentFragment fragment)
        {
          // TODO: coming soon.
        }
      },
  TRANSACTION_STARTED
      {
        @Override
        void activate(@NonNull BookmarkPaymentFragment fragment)
        {
          fragment.launchBillingFlow();
        }
      };

  abstract void activate(@NonNull BookmarkPaymentFragment fragment);
}
