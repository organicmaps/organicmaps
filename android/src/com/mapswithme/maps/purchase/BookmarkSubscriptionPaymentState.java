package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;

import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

public enum  BookmarkSubscriptionPaymentState
{
  NONE
      {
        @Override
        void activate(@NonNull BookmarkSubscriptionFragment fragment)
        {
          throw new UnsupportedOperationException("This state can't bu used!");
        }
      },
  PRODUCT_DETAILS_LOADING
      {
        @Override
        void activate(@NonNull BookmarkSubscriptionFragment fragment)
        {
          showProgress(fragment);
          fragment.queryProductDetails();
        }
      },
  PRODUCT_DETAILS_LOADED
      {
        @Override
        void activate(@NonNull BookmarkSubscriptionFragment fragment)
        {
          hideProgress(fragment);
          // TODO: coming soon.
        }
      },
  PRODUCT_DETAILS_FAILURE
      {
        @Override
        void activate(@NonNull BookmarkSubscriptionFragment fragment)
        {
          PurchaseUtils.showProductDetailsFailureDialog(fragment, name());
        }
      },
  PAYMENT_FAILURE
      {
        @Override
        void activate(@NonNull BookmarkSubscriptionFragment fragment)
        {
          PurchaseUtils.showPaymentFailureDialog(fragment, name());
        }
      },
  PRICE_SELECTION
      {
        @Override
        void activate(@NonNull BookmarkSubscriptionFragment fragment)
        {
          hideProgress(fragment);
          fragment.updatePaymentButtons();
        }
      },
  VALIDATION
      {
        @Override
        void activate(@NonNull BookmarkSubscriptionFragment fragment)
        {
          UiUtils.hide(fragment.getViewOrThrow(), R.id.continue_btn);
          UiUtils.show(fragment.getViewOrThrow(), R.id.progress);
        }
      },
  VALIDATION_FINISH
      {
        @Override
        void activate(@NonNull BookmarkSubscriptionFragment fragment)
        {
          UiUtils.hide(fragment.getViewOrThrow(), R.id.progress);
          UiUtils.show(fragment.getViewOrThrow(), R.id.continue_btn);
          fragment.finishValidation();
        }
      };

  private static void showProgress(@NonNull BookmarkSubscriptionFragment fragment)
  {
    UiUtils.show(fragment.getViewOrThrow(), R.id.root_screen_progress);
    UiUtils.hide(fragment.getViewOrThrow(), R.id.content_view);
  }

  private static void hideProgress(@NonNull BookmarkSubscriptionFragment fragment)
  {
    UiUtils.hide(fragment.getViewOrThrow(), R.id.root_screen_progress);
    UiUtils.show(fragment.getViewOrThrow(), R.id.content_view);
  }

  abstract void activate(@NonNull BookmarkSubscriptionFragment fragment);
}
