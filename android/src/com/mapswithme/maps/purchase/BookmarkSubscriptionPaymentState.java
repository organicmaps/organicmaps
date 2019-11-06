package com.mapswithme.maps.purchase;

import androidx.annotation.NonNull;

public enum  BookmarkSubscriptionPaymentState
{
  NONE
      {
        @Override
        void activate(@NonNull SubscriptionUiChangeListener listener)
        {
          listener.onReset();
        }
      },
  PRODUCT_DETAILS_LOADING
      {
        @Override
        void activate(@NonNull SubscriptionUiChangeListener listener)
        {
          listener.onProductDetailsLoading();
        }
      },
  PRODUCT_DETAILS_FAILURE
      {
        @Override
        void activate(@NonNull SubscriptionUiChangeListener listener)
        {
          listener.onProductDetailsFailure();
        }
      },
  PAYMENT_FAILURE
      {
        @Override
        void activate(@NonNull SubscriptionUiChangeListener listener)
        {
          listener.onPaymentFailure();
        }
      },
  PRICE_SELECTION
      {
        @Override
        void activate(@NonNull SubscriptionUiChangeListener listener)
        {
          listener.onPriceSelection();
        }
      },
  VALIDATION
      {
        @Override
        void activate(@NonNull SubscriptionUiChangeListener listener)
        {
          listener.onValidating();
        }
      },
  VALIDATION_FINISH
      {
        @Override
        void activate(@NonNull SubscriptionUiChangeListener listener)
        {
          listener.onValidationFinish();
        }
      },
  PINGING
      {
        @Override
        void activate(@NonNull SubscriptionUiChangeListener listener)
        {
          listener.onPinging();
        }
      },
  PINGING_FINISH
      {
        @Override
        void activate(@NonNull SubscriptionUiChangeListener listener)
        {
          listener.onPingFinish();
        }
      },

  CHECK_NETWORK_CONNECTION
      {
        @Override
        void activate(@NonNull SubscriptionUiChangeListener listener)
        {
          listener.onCheckNetworkConnection();

        }
      };

  abstract void activate(@NonNull SubscriptionUiChangeListener listener);
}
