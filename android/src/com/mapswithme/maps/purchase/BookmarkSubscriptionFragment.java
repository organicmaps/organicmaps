package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.CardView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.widget.Toast;

import com.android.billingclient.api.SkuDetails;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.PrivateVariables;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseAuthFragment;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.dialog.AlertDialogCallback;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;

import java.util.Collections;
import java.util.List;

public class BookmarkSubscriptionFragment extends BaseAuthFragment
    implements AlertDialogCallback, PurchaseStateActivator<BookmarkSubscriptionPaymentState>
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private static final String TAG = BookmarkSubscriptionFragment.class.getSimpleName();
  private final static String EXTRA_CURRENT_STATE = "extra_current_state";
  private final static String EXTRA_PRODUCT_DETAILS = "extra_product_details";
  private static final int DEF_ELEVATION = 0;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private PurchaseController<PurchaseCallback> mPurchaseController;
  @NonNull
  private final BookmarkSubscriptionCallback mPurchaseCallback = new BookmarkSubscriptionCallback();
  @NonNull
  private final PingCallback mPingCallback = new PingCallback();
  @NonNull
  private BookmarkSubscriptionPaymentState mState = BookmarkSubscriptionPaymentState.NONE;
  @Nullable
  private ProductDetails[] mProductDetails;
  private boolean mValidationResult;
  private boolean mPingingResult;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    mPurchaseController = PurchaseFactory.createBookmarksSubscriptionPurchaseController(requireContext());
    if (savedInstanceState != null)
      mPurchaseController.onRestore(savedInstanceState);
    mPurchaseController.initialize(requireActivity());
    mPingCallback.attach(this);
    BookmarkManager.INSTANCE.addCatalogPingListener(mPingCallback);
    View root = inflater.inflate(R.layout.bookmark_subscription_fragment, container, false);
    CardView annualPriceCard = root.findViewById(R.id.annual_price_card);
    CardView monthlyPriceCard = root.findViewById(R.id.monthly_price_card);
    AnnualCardClickListener annualCardListener = new AnnualCardClickListener(monthlyPriceCard,
                                                                             annualPriceCard);
    annualPriceCard.setOnClickListener(annualCardListener);
    MonthlyCardClickListener monthlyCardListener = new MonthlyCardClickListener(monthlyPriceCard,
                                                                                annualPriceCard);
    monthlyPriceCard.setOnClickListener(monthlyCardListener);

    TextView restorePurchasesBtn = root.findViewById(R.id.restore_purchase_btn);
    restorePurchasesBtn.setOnClickListener(v -> openSubscriptionManagementSettings());

    View continueBtn = root.findViewById(R.id.continue_btn);
    continueBtn.setOnClickListener(v -> onContinueButtonClicked());

    annualPriceCard.setSelected(true);
    monthlyPriceCard.setSelected(false);
    annualPriceCard.setCardElevation(getResources().getDimension(R.dimen.margin_base_plus_quarter));

    Statistics.INSTANCE.trackPurchasePreviewShow(PrivateVariables.bookmarksSubscriptionServerId(),
                                                 PrivateVariables.bookmarksSubscriptionVendor(),
                                                 PrivateVariables.bookmarksSubscriptionYearlyProductId());
    return root;
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();
    mPingCallback.detach();
    BookmarkManager.INSTANCE.removeCatalogPingListener(mPingCallback);
  }

  private void openSubscriptionManagementSettings()
  {
    Utils.openUrl(requireContext(), "https://play.google.com/store/account/subscriptions");
    Statistics.INSTANCE.trackPurchaseEvent(Statistics.EventName.INAPP_PURCHASE_PREVIEW_RESTORE,
                                           PrivateVariables.bookmarksSubscriptionServerId());
  }

  private void onContinueButtonClicked()
  {
    BookmarkManager.INSTANCE.pingBookmarkCatalog();
    activateState(BookmarkSubscriptionPaymentState.PINGING);

    Statistics.INSTANCE.trackPurchaseEvent(Statistics.EventName.INAPP_PURCHASE_PREVIEW_PAY,
                                           PrivateVariables.bookmarksSubscriptionServerId());
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    if (savedInstanceState != null)
    {
      BookmarkSubscriptionPaymentState savedState
          = BookmarkSubscriptionPaymentState.values()[savedInstanceState.getInt(EXTRA_CURRENT_STATE)];
      ProductDetails[] productDetails
          = (ProductDetails[]) savedInstanceState.getParcelableArray(EXTRA_PRODUCT_DETAILS);
      if (productDetails != null)
        mProductDetails = productDetails;

      activateState(savedState);
      return;
    }

    activateState(BookmarkSubscriptionPaymentState.PRODUCT_DETAILS_LOADING);
    mPurchaseController.queryProductDetails();
  }

  void queryProductDetails()
  {
    mPurchaseController.queryProductDetails();
  }

  @Override
  public void onStart()
  {
    super.onStart();
    mPurchaseController.addCallback(mPurchaseCallback);
    mPurchaseCallback.attach(this);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    mPurchaseController.removeCallback();
    mPurchaseCallback.detach();
  }

  @Override
  public void activateState(@NonNull BookmarkSubscriptionPaymentState state)
  {
    if (state == mState)
      return;

    LOGGER.i(TAG, "Activate state: " + state);
    mState = state;
    mState.activate(this);
  }

  private void handleProductDetails(@NonNull List<SkuDetails> details)
  {
    mProductDetails = new ProductDetails[PurchaseUtils.Period.values().length];
    for (SkuDetails sku: details)
    {
      PurchaseUtils.Period period = PurchaseUtils.Period.valueOf(sku.getSubscriptionPeriod());
      mProductDetails[period.ordinal()] = PurchaseUtils.toProductDetails(sku);
    }
  }

  void updatePaymentButtons()
  {
    updateYearlyButton();
    updateMonthlyButton();
  }

  private void updateYearlyButton()
  {
    ProductDetails details = getProductDetailsForPeriod(PurchaseUtils.Period.P1Y);
    String price = Utils.formatCurrencyString(details.getPrice(), details.getCurrencyCode());
    TextView priceView = getViewOrThrow().findViewById(R.id.annual_price);
    priceView.setText(price);
    TextView savingView = getViewOrThrow().findViewById(R.id.sale);
    String text = getString(R.string.annual_save_component, calculateYearlySaving());
    savingView.setText(text);
  }

  private void updateMonthlyButton()
  {
    ProductDetails details = getProductDetailsForPeriod(PurchaseUtils.Period.P1M);
    String price = Utils.formatCurrencyString(details.getPrice(), details.getCurrencyCode());
    TextView priceView = getViewOrThrow().findViewById(R.id.monthly_price);
    priceView.setText(price);
  }

  private int calculateYearlySaving()
  {
    float pricePerMonth = getProductDetailsForPeriod(PurchaseUtils.Period.P1M).getPrice();
    float pricePerYear = getProductDetailsForPeriod(PurchaseUtils.Period.P1Y).getPrice();
    return (int) (100 * (1 - pricePerYear / (pricePerMonth * PurchaseUtils.MONTHS_IN_YEAR)));
  }

  @NonNull
  private ProductDetails getProductDetailsForPeriod(@NonNull PurchaseUtils.Period period)
  {
    if (mProductDetails == null)
      throw new AssertionError("Product details must be exist at this moment!");
    return mProductDetails[period.ordinal()];
  }

  @Override
  public void onAlertDialogPositiveClick(int requestCode, int which)
  {
    // TODO: coming soon.
  }

  @Override
  public void onAlertDialogNegativeClick(int requestCode, int which)
  {
    // TODO: coming soon.
  }

  @Override
  public void onAlertDialogCancel(int requestCode)
  {
    // TODO: coming soon.
  }

  private void handleActivationResult(boolean result)
  {
    mValidationResult = result;
  }

  private void handlePingingResult(boolean result)
  {
    mPingingResult = result;
  }

  void finishValidation()
  {
    if (mValidationResult)
      requireActivity().setResult(Activity.RESULT_OK);

    requireActivity().finish();
  }

  public void finishPinging()
  {
    if (!mPingingResult)
    {
      PurchaseUtils.showPingFailureDialog(this);
      return;
    }

    authorize();
  }

  @Override
  public boolean onBackPressed()
  {
    Statistics.INSTANCE.trackPurchaseEvent(Statistics.EventName.INAPP_PURCHASE_PREVIEW_CANCEL,
                                           PrivateVariables.bookmarksSubscriptionServerId());
    return super.onBackPressed();
  }

  private void launchPurchaseFlow()
  {
    CardView annualCard = getViewOrThrow().findViewById(R.id.annual_price_card);
    PurchaseUtils.Period period = annualCard.getCardElevation() > 0 ? PurchaseUtils.Period.P1Y
                                                                    : PurchaseUtils.Period.P1M;
    ProductDetails details = getProductDetailsForPeriod(period);
    mPurchaseController.launchPurchaseFlow(details.getProductId());
  }

  @Override
  protected int getProgressMessageId()
  {
    return R.string.please_wait;
  }

  @Override
  public void onAuthorizationFinish(boolean success)
  {
    hideProgress();
    if (!success)
    {
      Toast.makeText(requireContext(), R.string.profile_authorization_error, Toast.LENGTH_LONG)
           .show();
      return;
    }

    launchPurchaseFlow();
  }

  @Override
  public void onAuthorizationStart()
  {
    showProgress();
  }

  @Override
  public void onSocialAuthenticationCancel(@Framework.AuthTokenType int type)
  {
    LOGGER.i(TAG, "Social authentication cancelled,  auth type = " + type);
  }

  @Override
  public void onSocialAuthenticationError(@Framework.AuthTokenType int type, @Nullable String error)
  {
    LOGGER.w(TAG, "Social authentication error = " + error + ",  auth type = " + type);
  }

  private class AnnualCardClickListener implements View.OnClickListener
  {
    @NonNull
    private final CardView mMonthlyPriceCard;

    @NonNull
    private final CardView mAnnualPriceCard;

    AnnualCardClickListener(@NonNull CardView monthlyPriceCard,
                            @NonNull CardView annualPriceCard)
    {
      mMonthlyPriceCard = monthlyPriceCard;
      mAnnualPriceCard = annualPriceCard;
    }

    @Override
    public void onClick(View v)
    {
      mMonthlyPriceCard.setCardElevation(DEF_ELEVATION);
      mAnnualPriceCard.setCardElevation(getResources().getDimension(R.dimen.margin_base_plus_quarter));

      if (!mAnnualPriceCard.isSelected())
        Statistics.INSTANCE.trackPurchasePreviewSelect(PrivateVariables.bookmarksSubscriptionServerId(),
                                                       PrivateVariables.bookmarksSubscriptionYearlyProductId());

      mMonthlyPriceCard.setSelected(false);
      mAnnualPriceCard.setSelected(true);
    }
  }

  private class MonthlyCardClickListener implements View.OnClickListener
  {
    @NonNull
    private final CardView mMonthlyPriceCard;

    @NonNull
    private final CardView mAnnualPriceCard;

    MonthlyCardClickListener(@NonNull CardView monthlyPriceCard,
                             @NonNull CardView annualPriceCard)
    {
      mMonthlyPriceCard = monthlyPriceCard;
      mAnnualPriceCard = annualPriceCard;
    }

    @Override
    public void onClick(View v)
    {
      mMonthlyPriceCard.setCardElevation(getResources().getDimension(R.dimen.margin_base_plus_quarter));
      mAnnualPriceCard.setCardElevation(DEF_ELEVATION);

      if (!mMonthlyPriceCard.isSelected())
        Statistics.INSTANCE.trackPurchasePreviewSelect(PrivateVariables.bookmarksSubscriptionServerId(),
                                                       PrivateVariables.bookmarksSubscriptionMonthlyProductId());

      mMonthlyPriceCard.setSelected(true);
      mAnnualPriceCard.setSelected(false);
    }
  }

  private static class BookmarkSubscriptionCallback
      extends StatefulPurchaseCallback<BookmarkSubscriptionPaymentState, BookmarkSubscriptionFragment>
      implements PurchaseCallback
  {
    @Nullable
    private List<SkuDetails> mPendingDetails;
    private Boolean mPendingValidationResult;

    @Override
    public void onProductDetailsLoaded(@NonNull List<SkuDetails> details)
    {
      if (PurchaseUtils.hasIncorrectSkuDetails(details))
      {
        activateStateSafely(BookmarkSubscriptionPaymentState.PRODUCT_DETAILS_FAILURE);
        return;
      }

      if (getUiObject() == null)
        mPendingDetails = Collections.unmodifiableList(details);
      else
        getUiObject().handleProductDetails(details);
      activateStateSafely(BookmarkSubscriptionPaymentState.PRICE_SELECTION);
    }

    @Override
    public void onPaymentFailure(int error)
    {
      Statistics.INSTANCE.trackPurchaseStoreError(PrivateVariables.bookmarksSubscriptionServerId(),
                                                  error);
      activateStateSafely(BookmarkSubscriptionPaymentState.PAYMENT_FAILURE);
    }

    @Override
    public void onProductDetailsFailure()
    {
      activateStateSafely(BookmarkSubscriptionPaymentState.PRODUCT_DETAILS_FAILURE);
    }

    @Override
    public void onStoreConnectionFailed()
    {
      activateStateSafely(BookmarkSubscriptionPaymentState.PRODUCT_DETAILS_FAILURE);
    }

    @Override
    public void onValidationStarted()
    {
      Statistics.INSTANCE.trackPurchaseEvent(Statistics.EventName.INAPP_PURCHASE_STORE_SUCCESS,
                                             PrivateVariables.bookmarksSubscriptionServerId());
      activateStateSafely(BookmarkSubscriptionPaymentState.VALIDATION);
    }

    @Override
    public void onValidationFinish(boolean success)
    {
      if (getUiObject() == null)
        mPendingValidationResult = success;
      else
        getUiObject().handleActivationResult(success);

      activateStateSafely(BookmarkSubscriptionPaymentState.VALIDATION_FINISH);
    }

    @Override
    void onAttach(@NonNull BookmarkSubscriptionFragment bookmarkSubscriptionFragment)
    {
      if (mPendingDetails != null)
      {
        bookmarkSubscriptionFragment.handleProductDetails(mPendingDetails);
        mPendingDetails = null;
      }

      if (mPendingValidationResult != null)
      {
        bookmarkSubscriptionFragment.handleActivationResult(mPendingValidationResult);
        mPendingValidationResult = null;
      }
    }
  }

  private static class PingCallback
      extends StatefulPurchaseCallback<BookmarkSubscriptionPaymentState,
      BookmarkSubscriptionFragment> implements BookmarkManager.BookmarksCatalogPingListener

  {
    private Boolean mPendingPingingResult;

    @Override
    public void onPingFinished(boolean isServiceAvailable)
    {
      LOGGER.i(TAG, "Ping finished, isServiceAvailable: " + isServiceAvailable);
      if (getUiObject() == null)
        mPendingPingingResult = isServiceAvailable;
      else
        getUiObject().handlePingingResult(isServiceAvailable);

      activateStateSafely(BookmarkSubscriptionPaymentState.PINGING_FINISH);
    }

    @Override
    void onAttach(@NonNull BookmarkSubscriptionFragment fragment)
    {
      if (mPendingPingingResult != null)
      {
        fragment.handlePingingResult(mPendingPingingResult);
        mPendingPingingResult = null;
      }
    }
  }
}
