package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;
import android.support.v4.app.DialogFragment;
import android.view.View;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.util.UiUtils;

enum AdsRemovalPaymentState
{
  NONE
      {
        @Override
        void activate(@NonNull AdsRemovalPurchaseDialog dialog)
        {
          throw new UnsupportedOperationException("This state can't be used!");
        }
      },
  LOADING
      {
        @Override
        void activate(@NonNull AdsRemovalPurchaseDialog dialog)
        {
          View view = AdsRemovalPaymentState.getDialogViewOrThrow(dialog);
          UiUtils.hide(view, R.id.title, R.id.image, R.id.pay_button_container);
          View progressLayout = view.findViewById(R.id.progress_layout);
          TextView message = progressLayout.findViewById(R.id.message);
          message.setText(R.string.purchase_loading);
          UiUtils.show(progressLayout);
          dialog.queryPurchaseDetails();
        }
      },
  PRICE_SELECTION
      {
        @Override
        void activate(@NonNull AdsRemovalPurchaseDialog dialog)
        {
          View view = AdsRemovalPaymentState.getDialogViewOrThrow(dialog);
          UiUtils.hide(view, R.id.progress_layout);
          UiUtils.show(view, R.id.title, R.id.image, R.id.pay_button_container);
          TextView title = view.findViewById(R.id.title);
          title.setText(R.string.remove_ads_title);
          dialog.updateYearlyButton();
        }
      },
  EXPLANATION
      {
        @Override
        void activate(@NonNull AdsRemovalPurchaseDialog dialog)
        {

        }
      },
  VALIDATION
      {
        @Override
        void activate(@NonNull AdsRemovalPurchaseDialog dialog)
        {
          View view = AdsRemovalPaymentState.getDialogViewOrThrow(dialog);
          UiUtils.hide(view, R.id.title, R.id.image, R.id.pay_button_container);
          View progressLayout = view.findViewById(R.id.progress_layout);
          TextView message = progressLayout.findViewById(R.id.message);
          message.setText(R.string.please_wait);
          UiUtils.show(progressLayout);
        }
      };

  @NonNull
  private static View getDialogViewOrThrow(@NonNull DialogFragment dialog)
  {
    View view = dialog.getView();
    if (view == null)
      throw new IllegalStateException("Before call this method make sure that the dialog exists");
    return view;
  }

  abstract void activate(@NonNull AdsRemovalPurchaseDialog dialog);
}
