package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import com.android.billingclient.api.SkuDetails;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseAuthFragment;
import com.mapswithme.maps.bookmarks.AuthBundleFactory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.dialog.AlertDialog;
import com.mapswithme.maps.dialog.AlertDialogCallback;
import com.mapswithme.maps.dialog.ResolveFragmentManagerStrategy;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

abstract class AbstractBookmarkSubscriptionFragment extends BaseAuthFragment
    implements PurchaseStateActivator<BookmarkSubscriptionPaymentState>,
               SubscriptionUiChangeListener, AlertDialogCallback
{
  static final String EXTRA_FROM = "extra_from";

  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private static final String TAG = AbstractBookmarkSubscriptionFragment.class.getSimpleName();
  private static final String EXTRA_CURRENT_STATE = "extra_current_state";
  private static final String EXTRA_PRODUCT_DETAILS = "extra_product_details";

  private boolean mPingingResult;
  private boolean mValidationResult;
  @NonNull
  private final PingCallback mPingCallback = new PingCallback();
  @SuppressWarnings("NullableProblems")
  @NonNull
  private BookmarkSubscriptionCallback mPurchaseCallback;
  @NonNull
  private BookmarkSubscriptionPaymentState mState = BookmarkSubscriptionPaymentState.NONE;
  @Nullable
  private ProductDetails[] mProductDetails;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private PurchaseController<PurchaseCallback> mPurchaseController;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private SubscriptionFragmentDelegate mDelegate;

  @Nullable
  @Override
  public final View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                                 @Nullable Bundle savedInstanceState)
  {
    mPurchaseCallback = new BookmarkSubscriptionCallback(getSubscriptionType());
    mPurchaseController = createPurchaseController();
    if (savedInstanceState != null)
      mPurchaseController.onRestore(savedInstanceState);
    mPurchaseController.initialize(requireActivity());
    mPingCallback.attach(this);
    BookmarkManager.INSTANCE.addCatalogPingListener(mPingCallback);
    View root = onSubscriptionCreateView(inflater, container, savedInstanceState);
    mDelegate = createFragmentDelegate(this);
    if (root != null)
      mDelegate.onCreateView(root);

    return root;
  }

  @Nullable
  private String getExtraFrom()
  {
    if (getArguments() == null)
      return null;

    return getArguments().getString(EXTRA_FROM, null);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    if (savedInstanceState != null)
    {
      BookmarkSubscriptionPaymentState savedState
          =
          BookmarkSubscriptionPaymentState.values()[savedInstanceState.getInt(EXTRA_CURRENT_STATE)];
      ProductDetails[] productDetails
          = (ProductDetails[]) savedInstanceState.getParcelableArray(EXTRA_PRODUCT_DETAILS);
      if (productDetails != null)
        mProductDetails = productDetails;

      activateState(savedState);
      return;
    }

    activateState(BookmarkSubscriptionPaymentState.CHECK_NETWORK_CONNECTION);
  }

  protected void authorize()
  {
    authorize(AuthBundleFactory.subscription());
  }

  @Override
  public final void onDestroyView()
  {
    super.onDestroyView();
    mPingCallback.detach();
    BookmarkManager.INSTANCE.removeCatalogPingListener(mPingCallback);
    mDelegate.onDestroyView();
  }

  @Override
  public final void onStart()
  {
    super.onStart();
    mPurchaseController.addCallback(mPurchaseCallback);
    mPurchaseCallback.attach(this);
  }

  @Override
  public final void onStop()
  {
    super.onStop();
    mPurchaseController.removeCallback();
    mPurchaseCallback.detach();
  }

  private void queryProductDetails()
  {
    mPurchaseController.queryProductDetails();
  }

  private void launchPurchaseFlow(@NonNull String productId)
  {
    mPurchaseController.launchPurchaseFlow(productId);
  }

  private void handleProductDetails(@NonNull List<SkuDetails> details)
  {
    mProductDetails = new ProductDetails[PurchaseUtils.Period.values().length];
    for (SkuDetails sku : details)
    {
      PurchaseUtils.Period period = PurchaseUtils.Period.valueOf(sku.getSubscriptionPeriod());
      mProductDetails[period.ordinal()] = PurchaseUtils.toProductDetails(sku);
    }
  }

  @NonNull
  ProductDetails getProductDetailsForPeriod(@NonNull PurchaseUtils.Period period)
  {
    if (mProductDetails == null)
      throw new AssertionError("Product details must be exist at this moment!");
    return mProductDetails[period.ordinal()];
  }

  @Nullable
  List<ProductDetails> getProductDetails()
  {
    return mProductDetails == null ? null
                                   : Collections.synchronizedList(Arrays.asList(mProductDetails));
  }

  @Override
  public void onAuthorizationFinish(boolean success)
  {
    hideProgress();
    if (!success)
    {
      Utils.showSnackbar(requireContext(), getViewOrThrow(), R.string.profile_authorization_error);
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
  protected int getProgressMessageId()
  {
    return R.string.please_wait;
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

  @Override
  public void activateState(@NonNull BookmarkSubscriptionPaymentState state)
  {
    if (state == mState)
      return;

    LOGGER.i(TAG, "Activate state: " + state);
    mState = state;
    mState.activate(this);
  }

  private void handlePingingResult(boolean result)
  {
    mPingingResult = result;
  }

  private void finishPinging()
  {
    if (!mPingingResult)
    {
      PurchaseUtils.showPingFailureDialog(this);
      return;
    }

    authorize();
  }

  @Override
  public void onCheckNetworkConnection()
  {
    if (ConnectionState.INSTANCE.isConnected())
    {
      onNetworkCheckPassed();
      return;
    }

    PurchaseUtils.showNoConnectionDialog(this);
  }

  @Override
  public void onProductDetailsFailure()
  {
    PurchaseUtils.showProductDetailsFailureDialog(this, getClass().getSimpleName());
  }

  @Override
  public void onPaymentFailure()
  {
    PurchaseUtils.showPaymentFailureDialog(this, getClass().getSimpleName());
  }

  private void onNetworkCheckPassed()
  {
    activateState(BookmarkSubscriptionPaymentState.PRODUCT_DETAILS_LOADING);
  }

  @Override
  public void onReset()
  {
    mDelegate.onReset();
  }

  @Override
  public void onPriceSelection()
  {
    mDelegate.onPriceSelection();
    ProductDetails details = getProductDetailsForPeriod(PurchaseUtils.Period.P1Y);
    boolean isTrial = !TextUtils.isEmpty(details.getFreeTrialPeriod());
    Statistics.INSTANCE.trackPurchasePreviewShow(getSubscriptionType().getServerId(),
                                                 getSubscriptionType().getVendor(),
                                                 getSubscriptionType().getYearlyProductId(),
                                                 getExtraFrom(), isTrial);
  }

  @Override
  @CallSuper
  public void onProductDetailsLoading()
  {
    queryProductDetails();
    mDelegate.onProductDetailsLoading();
  }

  @Override
  public void onPinging()
  {
    showButtonProgress();
  }

  @Override
  @CallSuper
  public void onPingFinish()
  {
    finishPinging();
    hideButtonProgress();
  }

  @Override
  @CallSuper
  public void onValidationFinish()
  {
    finishValidation();
    hideButtonProgress();
  }

  private void finishValidation()
  {
    if (mValidationResult)
      requireActivity().setResult(Activity.RESULT_OK);

    requireActivity().finish();
  }

  private void handleValidationResult(boolean result)
  {
    mValidationResult = result;
  }

  @Override
  public void onAlertDialogPositiveClick(int requestCode, int which)
  {
    if (requestCode == PurchaseUtils.REQ_CODE_NO_NETWORK_CONNECTION_DIALOG)
    {
      dismissOutdatedNoNetworkDialog();
      activateState(BookmarkSubscriptionPaymentState.NONE);
      activateState(BookmarkSubscriptionPaymentState.CHECK_NETWORK_CONNECTION);
    }
  }

  private void dismissOutdatedNoNetworkDialog()
  {
    ResolveFragmentManagerStrategy strategy
        = AlertDialog.FragManagerStrategyType.ACTIVITY_FRAGMENT_MANAGER.getValue();
    FragmentManager manager = strategy.resolve(this);
    Fragment outdatedInstance =
        manager.findFragmentByTag(PurchaseUtils.NO_NETWORK_CONNECTION_DIALOG_TAG);
    if (outdatedInstance == null)
      return;
    manager.beginTransaction().remove(outdatedInstance).commitAllowingStateLoss();
    manager.executePendingTransactions();
  }

  @Override
  public void onAlertDialogNegativeClick(int requestCode, int which)
  {
    if (requestCode == PurchaseUtils.REQ_CODE_NO_NETWORK_CONNECTION_DIALOG)
      requireActivity().finish();
  }

  @Override
  public void onAlertDialogCancel(int requestCode)
  {
    if (requestCode == PurchaseUtils.REQ_CODE_NO_NETWORK_CONNECTION_DIALOG)
      requireActivity().finish();
  }

  @NonNull
  abstract PurchaseController<PurchaseCallback> createPurchaseController();

  @NonNull
  abstract SubscriptionFragmentDelegate createFragmentDelegate(
      @NonNull AbstractBookmarkSubscriptionFragment fragment);

  @Nullable
  abstract View onSubscriptionCreateView(@NonNull LayoutInflater inflater,
                                         @Nullable ViewGroup container,
                                         @Nullable Bundle savedInstanceState);

  @NonNull
  abstract SubscriptionType getSubscriptionType();

  @Override
  public void onValidating()
  {
    showButtonProgress();
  }

  private void showButtonProgress()
  {
    mDelegate.showButtonProgress();
  }

  private void hideButtonProgress()
  {
    mDelegate.hideButtonProgress();
  }

  @Override
  public boolean onBackPressed()
  {
    Statistics.INSTANCE.trackPurchaseEvent(Statistics.EventName.INAPP_PURCHASE_PREVIEW_CANCEL,
                                           getSubscriptionType().getServerId());
    return super.onBackPressed();
  }

  void pingBookmarkCatalog()
  {
    BookmarkManager.INSTANCE.pingBookmarkCatalog();
    activateState(BookmarkSubscriptionPaymentState.PINGING);
  }

  @NonNull
  private PurchaseUtils.Period getSelectedPeriod()
  {
    return mDelegate.getSelectedPeriod();
  }

  void trackPayEvent()
  {
    Statistics.INSTANCE.trackPurchaseEvent(Statistics.EventName.INAPP_PURCHASE_PREVIEW_PAY,
                                           getSubscriptionType().getServerId(),
                                           Statistics.STATISTICS_CHANNEL_REALTIME);
  }

  private void launchPurchaseFlow()
  {
    ProductDetails details = getProductDetailsForPeriod(getSelectedPeriod());
    launchPurchaseFlow(details.getProductId());
  }

  int calculateYearlySaving()
  {
    float pricePerMonth = getProductDetailsForPeriod(PurchaseUtils.Period.P1M).getPrice();
    float pricePerYear = getProductDetailsForPeriod(PurchaseUtils.Period.P1Y).getPrice();
    return (int) (100 * (1 - pricePerYear / (pricePerMonth * PurchaseUtils.MONTHS_IN_YEAR)));
  }

  void trackYearlyProductSelected()
  {
    ProductDetails details = getProductDetailsForPeriod(PurchaseUtils.Period.P1Y);
    boolean isTrial = !TextUtils.isEmpty(details.getFreeTrialPeriod());
    Statistics.INSTANCE.trackPurchasePreviewSelect(getSubscriptionType().getServerId(),
                                                   getSubscriptionType().getYearlyProductId(),
                                                   isTrial);
  }

  void trackMonthlyProductSelected()
  {
    Statistics.INSTANCE.trackPurchasePreviewSelect(getSubscriptionType().getServerId(),
                                                   getSubscriptionType().getMonthlyProductId());
  }

  private static class PingCallback
      extends StatefulPurchaseCallback<BookmarkSubscriptionPaymentState,
      AbstractBookmarkSubscriptionFragment> implements BookmarkManager.BookmarksCatalogPingListener

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
    void onAttach(@NonNull AbstractBookmarkSubscriptionFragment fragment)
    {
      if (mPendingPingingResult != null)
      {
        fragment.handlePingingResult(mPendingPingingResult);
        mPendingPingingResult = null;
      }
    }
  }

  private static class BookmarkSubscriptionCallback
      extends StatefulPurchaseCallback<BookmarkSubscriptionPaymentState,
      AbstractBookmarkSubscriptionFragment>
      implements PurchaseCallback
  {
    @NonNull
    private final SubscriptionType mType;
    @Nullable
    private List<SkuDetails> mPendingDetails;
    private Boolean mPendingValidationResult;

    private BookmarkSubscriptionCallback(@NonNull SubscriptionType type)
    {
      mType = type;
    }

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
      Statistics.INSTANCE.trackPurchaseStoreError(mType.getServerId(), error);
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
                                             mType.getServerId());
      activateStateSafely(BookmarkSubscriptionPaymentState.VALIDATION);
    }

    @Override
    public void onValidationFinish(boolean success)
    {
      if (getUiObject() == null)
        mPendingValidationResult = success;
      else
        getUiObject().handleValidationResult(success);

      activateStateSafely(BookmarkSubscriptionPaymentState.VALIDATION_FINISH);
    }

    @Override
    void onAttach(@NonNull AbstractBookmarkSubscriptionFragment fragment)
    {
      if (mPendingDetails != null)
      {
        fragment.handleProductDetails(mPendingDetails);
        mPendingDetails = null;
      }

      if (mPendingValidationResult != null)
      {
        fragment.handleValidationResult(mPendingValidationResult);
        mPendingValidationResult = null;
      }
    }
  }
}
