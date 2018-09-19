package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;
import android.support.v4.app.DialogFragment;
import android.view.View;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.dialog.AlertDialog;
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
      },
   PAYMENT_FAILURE
       {
         @Override
         void activate(@NonNull AdsRemovalPurchaseDialog dialog)
         {
           AlertDialog.show(R.string.bookmarks_convert_error_title, R.string.purchase_error_subtitle,
                            R.string.back, dialog, AdsRemovalPurchaseDialog.REQ_CODE_PAYMENT_FAILURE);
         }
       },
   PRODUCT_DETAILS_FAILURE
       {
         @Override
         void activate(@NonNull AdsRemovalPurchaseDialog dialog)
         {
           AlertDialog.show(R.string.bookmarks_convert_error_title,
                            R.string.discovery_button_other_error_message, R.string.ok,
                            dialog, AdsRemovalPurchaseDialog.REQ_CODE_PRODUCT_DETAILS_FAILURE);
         }
       },
    VALIDATION_SERVER_ERROR
        {
          @Override
          void activate(@NonNull AdsRemovalPurchaseDialog dialog)
          {
            AlertDialog.show(R.string.server_unavailable_title, R.string.server_unavailable_message,
                             R.string.ok, dialog,
                             AdsRemovalPurchaseDialog.REQ_CODE_VALIDATION_SERVER_ERROR);
          }
        },
    VALIDATION_FINISH
        {
          @Override
          void activate(@NonNull AdsRemovalPurchaseDialog dialog)
          {
            dialog.dismissAllowingStateLoss();
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
