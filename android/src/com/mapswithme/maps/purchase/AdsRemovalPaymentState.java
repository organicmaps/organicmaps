package com.mapswithme.maps.purchase;

import android.support.annotation.IdRes;
import android.support.annotation.NonNull;
import android.support.v4.app.DialogFragment;
import android.view.View;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.mapswithme.maps.PrivateVariables;
import com.mapswithme.maps.R;
import com.mapswithme.maps.dialog.AlertDialog;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.Statistics;

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
          View view = getDialogViewOrThrow(dialog);
          UiUtils.hide(view, R.id.title, R.id.image, R.id.pay_button_container, R.id.explanation,
                       R.id.explanation_items);
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
          View view = getDialogViewOrThrow(dialog);
          UiUtils.hide(view, R.id.progress_layout, R.id.explanation_items);
          UiUtils.show(view, R.id.title, R.id.image, R.id.pay_button_container, R.id.explanation);
          TextView title = view.findViewById(R.id.title);
          title.setText(R.string.remove_ads_title);
          View image = view.findViewById(R.id.image);
          alignPayButtonBelow(view, image == null ? R.id.title : R.id.image);
          dialog.updatePaymentButtons();
          Statistics.INSTANCE.trackPurchasePreviewShow(PrivateVariables.adsRemovalServerId(),
                                                       PrivateVariables.adsRemovalVendor(),
                                                       PrivateVariables.adsRemovalYearlyProductId());
        }
      },
  EXPLANATION
      {
        @Override
        void activate(@NonNull AdsRemovalPurchaseDialog dialog)
        {
          View view = getDialogViewOrThrow(dialog);
          UiUtils.hide(view, R.id.image, R.id.explanation, R.id.progress_layout);
          UiUtils.show(view, R.id.title, R.id.explanation_items, R.id.pay_button_container);
          TextView title = view.findViewById(R.id.title);
          title.setText(R.string.why_support);
          alignPayButtonBelow(view, R.id.explanation_items);
          dialog.updatePaymentButtons();
        }
      },
  VALIDATION
      {
        @Override
        void activate(@NonNull AdsRemovalPurchaseDialog dialog)
        {
          View view = getDialogViewOrThrow(dialog);
          UiUtils.hide(view, R.id.title, R.id.image, R.id.pay_button_container, R.id.explanation,
                       R.id.explanation_items);
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

          AlertDialog alertDialog = new AlertDialog.Builder()
              .setReqCode(AdsRemovalPurchaseDialog.REQ_CODE_PAYMENT_FAILURE)
              .setTitleId(R.string.bookmarks_convert_error_title)
              .setMessageId(R.string.purchase_error_subtitle)
              .setPositiveBtnId(R.string.back)
              .build();
          alertDialog.show(dialog, name());
        }
      },
  PRODUCT_DETAILS_FAILURE
      {
        @Override
        void activate(@NonNull AdsRemovalPurchaseDialog dialog)
        {
          AlertDialog alertDialog = new AlertDialog.Builder()
              .setReqCode(AdsRemovalPurchaseDialog.REQ_CODE_PRODUCT_DETAILS_FAILURE)
              .setTitleId(R.string.bookmarks_convert_error_title)
              .setMessageId(R.string.discovery_button_other_error_message)
              .setPositiveBtnId(R.string.ok)
              .build();
          alertDialog.show(dialog, name());
        }
      },
  VALIDATION_FINISH
      {
        @Override
        void activate(@NonNull AdsRemovalPurchaseDialog dialog)
        {
          dialog.finishValidation();
        }
      };

  private static void alignPayButtonBelow(@NonNull View view, @IdRes int anchor)
  {
    View payButton = view.findViewById(R.id.pay_button_container);
    RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) payButton.getLayoutParams();
    params.addRule(RelativeLayout.BELOW, anchor);
  }

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
