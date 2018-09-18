package com.mapswithme.maps.purchase;

import android.content.Context;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.android.billingclient.api.SkuDetails;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.maps.dialog.AlertDialog;
import com.mapswithme.maps.dialog.AlertDialogCallback;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.List;

public class AdsRemovalPurchaseDialog extends BaseMwmDialogFragment implements AlertDialogCallback
{
  private final static Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private final static String TAG = AdsRemovalPurchaseDialog.class.getSimpleName();
  private final static String EXTRA_CURRENT_STATE = "extra_current_state";
  private final static String EXTRA_PRODUCT_DETAILS = "extra_product_details";
  private final static int WEEKS_IN_YEAR = 52;
  private final static int WEEKS_IN_MONTH = 4;
  private final static int REQ_CODE_PRODUCT_DETAILS_FAILURE = 1;
  private final static int REQ_CODE_PAYMENT_FAILURE = 2;

  @Nullable
  private ProductDetails[] mProductDetails;
  @NonNull
  private AdsRemovalPaymentState mState = AdsRemovalPaymentState.NONE;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private PurchaseController<AdsRemovalPurchaseCallback> mController;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mPayButtonContainer;
  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    LOGGER.d(TAG, "onCreate savedInstanceState = " + savedInstanceState);
  }

  @Override
  public void onAttach(Context context)
  {
    super.onAttach(context);
    LOGGER.d(TAG, "onAttach");
    mController = ((AdsRemovalPurchaseControllerProvider) context).getAdsRemovalPurchaseController();
    mController.addCallback(new PurchaseCallback());
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    LOGGER.d(TAG, "onCreateView savedInstanceState = " + savedInstanceState + "this " + this);
    View view = inflater.inflate(R.layout.fragment_ads_removal_purchase_dialog, container, false);
    mPayButtonContainer = view.findViewById(R.id.pay_button_container);
    mPayButtonContainer.setOnClickListener(v -> onYearlyProductClicked());
    return view;
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    LOGGER.d(TAG, "onViewCreated savedInstanceState = " + savedInstanceState);
    if (savedInstanceState != null)
    {
      AdsRemovalPaymentState savedState = AdsRemovalPaymentState.values()[savedInstanceState.getInt(EXTRA_CURRENT_STATE)];
      ProductDetails[] productDetails
          = (ProductDetails[]) savedInstanceState.getParcelableArray(EXTRA_PRODUCT_DETAILS);
      if (productDetails != null)
        mProductDetails = productDetails;

      activateState(savedState);
      return;
    }

    activateState(AdsRemovalPaymentState.LOADING);
  }

  void queryPurchaseDetails()
  {
    mController.queryPurchaseDetails();
  }

  void onYearlyProductClicked()
  {
    ProductDetails details = getProductDetailsForPeriod(Period.P1Y);
    mController.launchPurchaseFlow(details.getProductId());
  }

  private void activateState(@NonNull AdsRemovalPaymentState state)
  {
    mState = state;
    mState.activate(this);
  }

  @Override
  public void onSaveInstanceState(Bundle outState)
  {
    super.onSaveInstanceState(outState);
    LOGGER.d(TAG, "onSaveInstanceState");
    outState.putInt(EXTRA_CURRENT_STATE, mState.ordinal());
    outState.putParcelableArray(EXTRA_PRODUCT_DETAILS, mProductDetails);
  }

  @Override
  public void onDetach()
  {
    LOGGER.d(TAG, "onDetach");
    super.onDetach();
    mController.removeCallback();
  }

  void updateYearlyButton()
  {
    ProductDetails details = getProductDetailsForPeriod(Period.P1Y);
    String price = Utils.formatCurrencyString(details.getPrice(), details.getCurrencyCode());
    TextView priceView = mPayButtonContainer.findViewById(R.id.price);
    priceView.setText(getString(R.string.paybtn_title, price));
    TextView savingView = mPayButtonContainer.findViewById(R.id.saving);
    String saving = Utils.formatCurrencyString(calculateYearlySaving(), details.getCurrencyCode());
    savingView.setText(getString(R.string.paybtn_subtitle, saving));
  }

  @NonNull
  private ProductDetails getProductDetailsForPeriod(@NonNull Period period)
  {
    if (mProductDetails == null)
      throw new AssertionError("Product details must be exist at this moment!");
    return mProductDetails[period.ordinal()];
  }

  private float calculateYearlySaving()
  {
    float pricePerWeek = getProductDetailsForPeriod(Period.P1W).getPrice();
    float pricePerYear = getProductDetailsForPeriod(Period.P1Y).getPrice();
    return pricePerWeek * WEEKS_IN_YEAR - pricePerYear;
  }

  private float calculateMonthlySaving()
  {
    float pricePerWeek = getProductDetailsForPeriod(Period.P1W).getPrice();
    float pricePerMonth = getProductDetailsForPeriod(Period.P1M).getPrice();
    return pricePerWeek * WEEKS_IN_MONTH - pricePerMonth;
  }

  @Override
  public void onAlertDialogClick(int requestCode, int which)
  {
    handleErrorDialogEvent(requestCode);
  }

  @Override
  public void onAlertDialogCancel(int requestCode)
  {
    handleErrorDialogEvent(requestCode);
  }

  private void handleErrorDialogEvent(int requestCode)
  {
    switch (requestCode)
    {
      case REQ_CODE_PRODUCT_DETAILS_FAILURE:
        dismissAllowingStateLoss();
        break;
      case REQ_CODE_PAYMENT_FAILURE:
        activateState(AdsRemovalPaymentState.PRICE_SELECTION);
        break;
    }
  }

  private class PurchaseCallback implements AdsRemovalPurchaseCallback
  {
    @Override
    public void onProductDetailsLoaded(@NonNull List<SkuDetails> details)
    {
      mProductDetails = new ProductDetails[Period.values().length];
      for (SkuDetails sku: details)
      {
        float price = sku.getPriceAmountMicros() / 1000000;
        String currencyCode = sku.getPriceCurrencyCode();
        Period period = Period.valueOf(sku.getSubscriptionPeriod());
        mProductDetails[period.ordinal()] = new ProductDetails(sku.getSku(), price, currencyCode);
      }

      activateState(AdsRemovalPaymentState.PRICE_SELECTION);
    }

    @Override
    public void onPaymentFailure()
    {
      AlertDialog.show(R.string.bookmarks_convert_error_title, R.string.purchase_error_subtitle,
                       R.string.back, AdsRemovalPurchaseDialog.this, REQ_CODE_PAYMENT_FAILURE);
    }

    @Override
    public void onProductDetailsFailure()
    {
      AlertDialog.show(R.string.bookmarks_convert_error_title,
                       R.string.discovery_button_other_error_message, R.string.ok,
                       AdsRemovalPurchaseDialog.this, REQ_CODE_PRODUCT_DETAILS_FAILURE);
    }
  }

  enum Period
  {
    // Order is important.
    P1Y,
    P1M,
    P1W
  }
}
