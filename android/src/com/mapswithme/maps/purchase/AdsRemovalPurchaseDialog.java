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
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.List;

public class AdsRemovalPurchaseDialog extends BaseMwmDialogFragment
{
  private final static Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private final static String TAG = AdsRemovalPurchaseDialog.class.getSimpleName();
  private final static String EXTRA_CURRENT_STATE = "extra_current_state";
  private final static String EXTRA_PRODUCT_DETAILS = "extra_product_details";
  private final static int WEEKS_IN_YEAR = 52;
  private final static int WEEKS_IN_MONTH = 4;

  @NonNull
  private ProductDetails[] mProductDetails = new ProductDetails[Period.values().length];
  @NonNull
  private State mState = State.NONE;
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
      State savedState = State.values()[savedInstanceState.getInt(EXTRA_CURRENT_STATE)];
      ProductDetails[] productDetails
          = (ProductDetails[]) savedInstanceState.getParcelableArray(EXTRA_PRODUCT_DETAILS);
      if (productDetails != null && productDetails.length > 0)
      {
        mProductDetails = productDetails;
        updateYearlyButton();
      }
      activateState(savedState);
      return;
    }

    activateState(State.LOADING);
    mController.queryPurchaseDetails();
  }

  void onYearlyProductClicked()
  {
    ProductDetails details = getProductDetailsForPeriod(Period.P1Y);
    mController.launchPurchaseFlow(details.getProductId());
  }

  private void activateState(@NonNull State state)
  {
    mState = state;
    mState.activate(getViewOrThrow());
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

  private void updateYearlyButton()
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

  public enum State
  {
    NONE
        {
          @Override
          void activate(@NonNull View view)
          {
            throw new UnsupportedOperationException("This state can't be used!");
          }
        },
    LOADING
        {
          @Override
          void activate(@NonNull View view)
          {
            UiUtils.hide(view, R.id.title, R.id.image, R.id.pay_button_container);
            UiUtils.show(view, R.id.progress_layout);
          }
        },
    PRICE_SELECTION
        {
          @Override
          void activate(@NonNull View view)
          {
            UiUtils.hide(view, R.id.progress_layout);
            UiUtils.show(view, R.id.title, R.id.image, R.id.pay_button_container);
            TextView title = view.findViewById(R.id.title);
            title.setText(R.string.remove_ads_title);
          }
        },
    EXPLANATION
        {
          @Override
          void activate(@NonNull View view)
          {

          }
        },
    ERROR
        {
          @Override
          void activate(@NonNull View view)
          {

          }
        };

    abstract void activate(@NonNull View view);
  }

  private class PurchaseCallback implements AdsRemovalPurchaseCallback
  {
    @Override
    public void onProductDetailsLoaded(@NonNull List<SkuDetails> details)
    {
      for (SkuDetails sku: details)
      {
        float price = sku.getPriceAmountMicros() / 1000000;
        String currencyCode = sku.getPriceCurrencyCode();
        Period period = Period.valueOf(sku.getSubscriptionPeriod());
        mProductDetails[period.ordinal()] = new ProductDetails(sku.getSku(), price, currencyCode);
      }

      updateYearlyButton();
      activateState(State.PRICE_SELECTION);
    }

    @Override
    public void onFailure()
    {
      // Coming soon.
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
