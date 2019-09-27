package com.mapswithme.maps.purchase;

import androidx.annotation.NonNull;

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
              .setReqCode(PurchaseUtils.REQ_CODE_START_TRANSACTION_FAILURE)
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
          PurchaseUtils.showPaymentFailureDialog(fragment, name());
        }
      },
  VALIDATION
      {
        @Override
        void activate(@NonNull BookmarkPaymentFragment fragment)
        {
          showProgress(fragment);
          UiUtils.hide(fragment.getViewOrThrow(), R.id.cancel_btn);
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
          PurchaseUtils.showProductDetailsFailureDialog(fragment, name());
        }
      },
  SUBS_PRODUCT_DETAILS_FAILURE
      {
        @Override
        void activate(@NonNull BookmarkPaymentFragment fragment)
        {
          UiUtils.hide(fragment.getViewOrThrow(), R.id.buy_subs_container);
          PurchaseUtils.showProductDetailsFailureDialog(fragment, name());
        }
      },
  SUBS_PRODUCT_DETAILS_LOADED
      {
        @Override
        void activate(@NonNull BookmarkPaymentFragment fragment)
        {
          UiUtils.hide(fragment.getViewOrThrow(), R.id.subs_progress);
          fragment.updateSubsProductDetails();
        }
      };;

  private static void showProgress(@NonNull BookmarkPaymentFragment fragment)
  {
    UiUtils.show(fragment.getViewOrThrow(), R.id.inapp_progress);
    UiUtils.hide(fragment.getViewOrThrow(), R.id.buy_inapp_btn);
  }

  private static void hideProgress(@NonNull BookmarkPaymentFragment fragment)
  {
    UiUtils.hide(fragment.getViewOrThrow(), R.id.inapp_progress);
    UiUtils.show(fragment.getViewOrThrow(), R.id.buy_inapp_btn);
  }

  abstract void activate(@NonNull BookmarkPaymentFragment fragment);
}
