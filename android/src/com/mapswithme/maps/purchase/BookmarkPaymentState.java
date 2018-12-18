package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

import com.mapswithme.maps.R;
import com.mapswithme.maps.dialog.AlertDialog;
import com.mapswithme.util.UiUtils;

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
  PRODUCT_DETAILS_LOADING
      {
        @Override
        void activate(@NonNull BookmarkPaymentFragment fragment)
        {
          showProgress(fragment);
        }
      },
  PRODUCT_DETAILS_LOADED
      {
        @Override
        void activate(@NonNull BookmarkPaymentFragment fragment)
        {
          hideProgress(fragment);
          fragment.updateProductDetails();
        }
      },
  TRANSACTION_STARTING
      {
        @Override
        void activate(@NonNull BookmarkPaymentFragment fragment)
        {
          showProgress(fragment);
        }
      },
  TRANSACTION_FAILURE
      {
        @Override
        void activate(@NonNull BookmarkPaymentFragment fragment)
        {
          UiUtils.hide(fragment.getViewOrThrow(), R.id.progress);
          AlertDialog alertDialog = new AlertDialog.Builder()
              .setReqCode(BookmarkPaymentFragment.REQ_CODE_START_TRANSACTION_FAILURE)
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
          hideProgress(fragment);
          fragment.launchBillingFlow();
        }
      },
  PAYMENT_IN_PROGRESS
      {
        @Override
        void activate(@NonNull BookmarkPaymentFragment fragment)
        {
          // Do nothing by default.
        }
      },
  PAYMENT_FAILURE
      {
        @Override
        void activate(@NonNull BookmarkPaymentFragment fragment)
        {
          AlertDialog alertDialog = new AlertDialog.Builder()
              .setReqCode(BookmarkPaymentFragment.REQ_CODE_PAYMENT_FAILURE)
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
          showProgress(fragment);
        }
      },
  VALIDATION_FINISH
      {
        @Override
        void activate(@NonNull BookmarkPaymentFragment fragment)
        {
          hideProgress(fragment);
          fragment.finishValidation();
        }
      },
  PRODUCT_DETAILS_FAILURE
      {
        @Override
        void activate(@NonNull BookmarkPaymentFragment fragment)
        {
          AlertDialog alertDialog = new AlertDialog.Builder()
              .setReqCode(BookmarkPaymentFragment.REQ_CODE_PRODUCT_DETAILS_FAILURE)
              .setTitleId(R.string.bookmarks_convert_error_title)
              .setMessageId(R.string.discovery_button_other_error_message)
              .setPositiveBtnId(R.string.ok)
              .build();
          alertDialog.show(fragment, name());
        }
      };

  private static void showProgress(@NonNull BookmarkPaymentFragment fragment)
  {
    UiUtils.show(fragment.getViewOrThrow(), R.id.progress);
    UiUtils.hide(fragment.getViewOrThrow(), R.id.buy_btn);
  }

  private static void hideProgress(@NonNull BookmarkPaymentFragment fragment)
  {
    UiUtils.hide(fragment.getViewOrThrow(), R.id.progress);
    UiUtils.show(fragment.getViewOrThrow(), R.id.buy_btn);
  }

  abstract void activate(@NonNull BookmarkPaymentFragment fragment);
}
