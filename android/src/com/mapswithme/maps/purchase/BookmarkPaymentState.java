package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

import com.mapswithme.maps.R;
import com.mapswithme.maps.dialog.AlertDialog;

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
          fragment.showProgress();
        }
      },
  TRANSACTION_FAILURE
      {
        @Override
        void activate(@NonNull BookmarkPaymentFragment fragment)
        {
          fragment.hideProgress();
          AlertDialog alertDialog = new AlertDialog.Builder()
              .setTitleId(R.string.error_server_title)
              .setMessageId(R.string.error_server_message)
              .setPositiveBtnId(R.string.ok)
              .build();
          alertDialog.show(fragment, name());
        }
      },
  TRANSACTION_STARTED
      {
        @Override
        void activate(@NonNull BookmarkPaymentFragment fragment)
        {
          fragment.hideProgress();
          fragment.launchBillingFlow();
        }
      },
  PAYMENT_FAILURE
      {
        @Override
        void activate(@NonNull BookmarkPaymentFragment fragment)
        {
          AlertDialog alertDialog = new AlertDialog.Builder()
              .setTitleId(R.string.bookmarks_convert_error_title)
              .setMessageId(R.string.purchase_error_subtitle)
              .setPositiveBtnId(R.string.back)
              .build();
          alertDialog.show(fragment, name());
        }
      },
  VALIDATION
      {
        @Override
        void activate(@NonNull BookmarkPaymentFragment fragment)
        {
          fragment.showProgress();
        }
      },
  VALIDATION_FINISH
      {
        @Override
        void activate(@NonNull BookmarkPaymentFragment fragment)
        {
          // TODO: coming soon.
        }
      },
  PRODUCT_DETAILS_FAILURE
      {
        @Override
        void activate(@NonNull BookmarkPaymentFragment fragment)
        {
          AlertDialog alertDialog = new AlertDialog.Builder()
              .setTitleId(R.string.bookmarks_convert_error_title)
              .setMessageId(R.string.discovery_button_other_error_message)
              .setPositiveBtnId(R.string.ok)
              .build();
          alertDialog.show(fragment, name());
        }
      };

  abstract void activate(@NonNull BookmarkPaymentFragment fragment);
}
