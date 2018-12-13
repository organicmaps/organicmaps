package com.mapswithme.maps.purchase;

import android.content.Context;
import android.content.DialogInterface;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.TextView;

import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.SkuDetails;
import com.mapswithme.maps.PrivateVariables;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.maps.dialog.AlertDialogCallback;
import com.mapswithme.util.CrashlyticsUtils;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class AdsRemovalPurchaseDialog extends BaseMwmDialogFragment
    implements AlertDialogCallback, PurchaseStateActivator<AdsRemovalPaymentState>
{
  private final static Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private final static String TAG = AdsRemovalPurchaseDialog.class.getSimpleName();
  private final static String EXTRA_CURRENT_STATE = "extra_current_state";
  private final static String EXTRA_PRODUCT_DETAILS = "extra_product_details";
  private final static String EXTRA_ACTIVATION_RESULT = "extra_activation_result";
  private final static int WEEKS_IN_YEAR = 52;
  private final static int MONTHS_IN_YEAR = 12;
  final static int REQ_CODE_PRODUCT_DETAILS_FAILURE = 1;
  final static int REQ_CODE_PAYMENT_FAILURE = 2;
  final static int REQ_CODE_VALIDATION_SERVER_ERROR = 3;

  private boolean mActivationResult;
  @Nullable
  private ProductDetails[] mProductDetails;
  @NonNull
  private AdsRemovalPaymentState mState = AdsRemovalPaymentState.NONE;
  @Nullable
  private AdsRemovalPurchaseControllerProvider mControllerProvider;
  @NonNull
  private AdsRemovalPurchaseCallback mPurchaseCallback = new AdsRemovalPurchaseCallback();
  @NonNull
  private List<AdsRemovalActivationCallback> mActivationCallbacks = new ArrayList<>();
  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mYearlyButton;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mMonthlyButton;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mWeeklyButton;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private CompoundButton mOptionsButton;

  public static void show(@NonNull FragmentActivity context)
  {
    show(context.getSupportFragmentManager());
  }

  public static void show(@NonNull Fragment parent)
  {
    show(parent.getChildFragmentManager());
  }

  private static void show(@NonNull FragmentManager manager)
  {
    DialogFragment fragment = new AdsRemovalPurchaseDialog();
    fragment.show(manager, AdsRemovalPurchaseDialog.class.getName());
  }

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
    if (context instanceof AdsRemovalPurchaseControllerProvider)
      mControllerProvider = (AdsRemovalPurchaseControllerProvider) context;
    if (getParentFragment() instanceof  AdsRemovalPurchaseControllerProvider)
      mControllerProvider = (AdsRemovalPurchaseControllerProvider) getParentFragment();
    if (context instanceof AdsRemovalActivationCallback)
      mActivationCallbacks.add((AdsRemovalActivationCallback) context);
    if (getParentFragment() instanceof AdsRemovalActivationCallback)
      mActivationCallbacks.add((AdsRemovalActivationCallback) getParentFragment());
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    LOGGER.d(TAG, "onCreateView savedInstanceState = " + savedInstanceState + "this " + this);
    View view = inflater.inflate(R.layout.fragment_ads_removal_purchase_dialog, container, false);
    View payButtonContainer = view.findViewById(R.id.pay_button_container);
    mYearlyButton = payButtonContainer.findViewById(R.id.yearly_button);
    mYearlyButton.setOnClickListener(v -> onYearlyProductClicked());
    mMonthlyButton = payButtonContainer.findViewById(R.id.monthly_button);
    mMonthlyButton.setOnClickListener(v -> onMonthlyProductClicked());
    mWeeklyButton = payButtonContainer.findViewById(R.id.weekly_button);
    mWeeklyButton.setOnClickListener(v -> onWeeklyProductClicked());
    mOptionsButton = payButtonContainer.findViewById(R.id.options);
    mOptionsButton.setCompoundDrawablesWithIntrinsicBounds(null, null, getOptionsToggle(), null);
    mOptionsButton.setOnCheckedChangeListener((buttonView, isChecked) -> onOptionsClicked());
    view.findViewById(R.id.explanation).setOnClickListener(v -> onExplanationClick());
    return view;
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    LOGGER.d(TAG, "onViewCreated savedInstanceState = " + savedInstanceState);
    if (savedInstanceState != null)
    {
      AdsRemovalPaymentState savedState
          = AdsRemovalPaymentState.values()[savedInstanceState.getInt(EXTRA_CURRENT_STATE)];
      ProductDetails[] productDetails
          = (ProductDetails[]) savedInstanceState.getParcelableArray(EXTRA_PRODUCT_DETAILS);
      if (productDetails != null)
        mProductDetails = productDetails;
      mActivationResult = savedInstanceState.getBoolean(EXTRA_ACTIVATION_RESULT);

      activateState(savedState);
      return;
    }

    activateState(AdsRemovalPaymentState.LOADING);
  }

  @NonNull
  private Drawable getOptionsToggle()
  {
    return Graphics.tint(getContext(), mOptionsButton.isChecked() ? R.drawable.ic_expand_less
                                                                  : R.drawable.ic_expand_more,
                         android.R.attr.textColorSecondary);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    getControllerOrThrow().addCallback(mPurchaseCallback);
    mPurchaseCallback.attach(this);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    getControllerOrThrow().removeCallback();
    mPurchaseCallback.detach();
  }

  void queryPurchaseDetails()
  {
    getControllerOrThrow().queryPurchaseDetails();
  }

  void onYearlyProductClicked()
  {
    launchPurchaseFlowForPeriod(Period.P1Y);
  }

  void onMonthlyProductClicked()
  {
    launchPurchaseFlowForPeriod(Period.P1M);
  }

  void onWeeklyProductClicked()
  {
    launchPurchaseFlowForPeriod(Period.P1W);
  }

  void onOptionsClicked()
  {
    mOptionsButton.setCompoundDrawablesWithIntrinsicBounds(null, null, getOptionsToggle(), null);
    View payContainer = getViewOrThrow().findViewById(R.id.pay_button_container);
    UiUtils.showIf(mOptionsButton.isChecked(), payContainer, R.id.monthly_button,
                   R.id.weekly_button);
  }

  private void launchPurchaseFlowForPeriod(@NonNull Period period)
  {
    ProductDetails details = getProductDetailsForPeriod(period);
    getControllerOrThrow().launchPurchaseFlow(details.getProductId());
    String purchaseId = PrivateVariables.adsRemovalServerId();
    Statistics.INSTANCE.trackPurchasePreviewSelect(PrivateVariables.adsRemovalServerId(),
                                                   details.getProductId());
    Statistics.INSTANCE.trackPurchaseEvent(Statistics.EventName.INAPP_PURCHASE_PREVIEW_PAY,
                                           purchaseId);
  }

  void onExplanationClick()
  {
    activateState(AdsRemovalPaymentState.EXPLANATION);
  }

  @Override
  public void activateState(@NonNull AdsRemovalPaymentState state)
  {
    if (state == mState)
      return;

    LOGGER.i(TAG, "Activate state: " + state);
    mState = state;
    mState.activate(this);
  }

  @NonNull
  private PurchaseController<PurchaseCallback> getControllerOrThrow()
  {
    if (mControllerProvider == null)
      throw new IllegalStateException("Controller provider must be non-null at this point!");

    PurchaseController<PurchaseCallback> controller
        = mControllerProvider.getAdsRemovalPurchaseController();
    if (controller == null)
      throw new IllegalStateException("Controller must be non-null at this point!");

    return controller;
  }

  @Override
  public void onSaveInstanceState(Bundle outState)
  {
    super.onSaveInstanceState(outState);
    LOGGER.d(TAG, "onSaveInstanceState");
    outState.putInt(EXTRA_CURRENT_STATE, mState.ordinal());
    outState.putParcelableArray(EXTRA_PRODUCT_DETAILS, mProductDetails);
    outState.putBoolean(EXTRA_ACTIVATION_RESULT, mActivationResult);
  }

  @Override
  public void onCancel(DialogInterface dialog)
  {
    super.onCancel(dialog);
    Statistics.INSTANCE.trackPurchaseEvent(Statistics.EventName.INAPP_PURCHASE_PREVIEW_CANCEL,
                                           PrivateVariables.adsRemovalServerId());
  }

  @Override
  public void onDetach()
  {
    LOGGER.d(TAG, "onDetach");
    mControllerProvider = null;
    mActivationCallbacks.clear();
    super.onDetach();
  }

  void finishValidation()
  {
    if (mActivationResult)
    {
      for (AdsRemovalActivationCallback callback : mActivationCallbacks)
        callback.onAdsRemovalActivation();
    }

    dismissAllowingStateLoss();
  }

  void updatePaymentButtons()
  {
    updateYearlyButton();
    updateMonthlyButton();
    updateWeeklyButton();
  }

  private void updateYearlyButton()
  {
    ProductDetails details = getProductDetailsForPeriod(Period.P1Y);
    String price = Utils.formatCurrencyString(details.getPrice(), details.getCurrencyCode());
    TextView priceView = mYearlyButton.findViewById(R.id.price);
    priceView.setText(getString(R.string.paybtn_title, price));
    TextView savingView = mYearlyButton.findViewById(R.id.saving);
    String saving = Utils.formatCurrencyString(calculateYearlySaving(), details.getCurrencyCode());
    savingView.setText(getString(R.string.paybtn_subtitle, saving));
  }

  private void updateMonthlyButton()
  {
    ProductDetails details = getProductDetailsForPeriod(Period.P1M);
    String price = Utils.formatCurrencyString(details.getPrice(), details.getCurrencyCode());
    String saving = Utils.formatCurrencyString(calculateMonthlySaving(), details.getCurrencyCode());
    mMonthlyButton.setText(getString(R.string.options_dropdown_item1, price, saving));
  }

  private void updateWeeklyButton()
  {
    ProductDetails details = getProductDetailsForPeriod(Period.P1W);
    String price = Utils.formatCurrencyString(details.getPrice(), details.getCurrencyCode());
    mWeeklyButton.setText(getString(R.string.options_dropdown_item2, price));
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
    return pricePerWeek * WEEKS_IN_YEAR - pricePerMonth * MONTHS_IN_YEAR;
  }

  @Override
  public void onAlertDialogPositiveClick(int requestCode, int which)
  {
    handleErrorDialogEvent(requestCode);
  }

  @Override
  public void onAlertDialogNegativeClick(int requestCode, int which)
  {
     // Do nothing by default.
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
      case REQ_CODE_VALIDATION_SERVER_ERROR:
        dismissAllowingStateLoss();
        break;
      case REQ_CODE_PAYMENT_FAILURE:
        activateState(AdsRemovalPaymentState.PRICE_SELECTION);
        break;
    }
  }

  private void handleProductDetails(@NonNull List<SkuDetails> details)
  {
    mProductDetails = new ProductDetails[Period.values().length];
    for (SkuDetails sku: details)
    {
      Period period = Period.valueOf(sku.getSubscriptionPeriod());
      mProductDetails[period.ordinal()] = PurchaseUtils.toProductDetails(sku);
    }
  }

  private void handleActivationResult(boolean result)
  {
    mActivationResult = result;
  }

  private static class AdsRemovalPurchaseCallback
      extends StatefulPurchaseCallback<AdsRemovalPaymentState, AdsRemovalPurchaseDialog>
      implements PurchaseCallback
  {
    @Nullable
    private List<SkuDetails> mPendingDetails;
    private Boolean mPendingActivationResult;

    @Override
    public void onProductDetailsLoaded(@NonNull List<SkuDetails> details)
    {
      if (hasIncorrectSkuDetails(details))
      {
        activateStateSafely(AdsRemovalPaymentState.PRODUCT_DETAILS_FAILURE);
        return;
      }

      if (getUiObject() == null)
        mPendingDetails = Collections.unmodifiableList(details);
      else
        getUiObject().handleProductDetails(details);
      activateStateSafely(AdsRemovalPaymentState.PRICE_SELECTION);
    }

    private static boolean hasIncorrectSkuDetails(@NonNull List<SkuDetails> skuDetails)
    {
      for (SkuDetails each : skuDetails)
      {
        if (AdsRemovalPurchaseDialog.Period.getInstance(each.getSubscriptionPeriod()) == null)
        {
          String msg = "Unsupported subscription period: '" + each.getSubscriptionPeriod() + "'";
          CrashlyticsUtils.logException(new IllegalStateException(msg));
          LOGGER.e(TAG, msg);
          return true;
        }
      }
      return false;
    }

    @Override
    public void onPaymentFailure(@BillingClient.BillingResponse int error)
    {
      Statistics.INSTANCE.trackPurchaseStoreError(PrivateVariables.adsRemovalServerId(), error);
      activateStateSafely(AdsRemovalPaymentState.PAYMENT_FAILURE);
    }

    @Override
    public void onProductDetailsFailure()
    {
      activateStateSafely(AdsRemovalPaymentState.PRODUCT_DETAILS_FAILURE);
    }

    @Override
    public void onStoreConnectionFailed()
    {
      activateStateSafely(AdsRemovalPaymentState.PRODUCT_DETAILS_FAILURE);
    }

    @Override
    public void onValidationStarted()
    {
      Statistics.INSTANCE.trackPurchaseEvent(Statistics.EventName.INAPP_PURCHASE_STORE_SUCCESS,
                                             PrivateVariables.adsRemovalServerId());
      activateStateSafely(AdsRemovalPaymentState.VALIDATION);
    }

    @Override
    public void onValidationFinish(boolean success)
    {
      if (getUiObject() == null)
        mPendingActivationResult = success;
      else
        getUiObject().handleActivationResult(success);

      activateStateSafely(AdsRemovalPaymentState.VALIDATION_FINISH);
    }

    @Override
    void onAttach(@NonNull AdsRemovalPurchaseDialog uiObject)
    {
      if (mPendingDetails != null)
      {
        uiObject.handleProductDetails(mPendingDetails);
        mPendingDetails = null;
      }

      if (mPendingActivationResult != null)
      {
        uiObject.handleActivationResult(mPendingActivationResult);
        mPendingActivationResult = null;
      }
    }
  }

  enum Period
  {
    // Order is important.
    P1Y,
    P1M,
    P1W;

    @Nullable
    static Period getInstance(@Nullable String subscriptionPeriod)
    {
      for (Period each : values())
      {
        if (TextUtils.equals(each.name(), subscriptionPeriod))
          return each;
      }
      return null;
    }
  }
}
