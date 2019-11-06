package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;

import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import com.android.billingclient.api.SkuDetails;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.PrivateVariables;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseAuthFragment;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.dialog.AlertDialog;
import com.mapswithme.maps.dialog.AlertDialogCallback;
import com.mapswithme.maps.dialog.ResolveFragmentManagerStrategy;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.NetworkPolicy;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;

import java.util.Collections;
import java.util.List;

abstract class AbstractBookmarkSubscriptionFragment extends BaseAuthFragment
    implements PurchaseStateActivator<BookmarkSubscriptionPaymentState>,
               SubscriptionUiChangeListener, AlertDialogCallback
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private static final String TAG = AbstractBookmarkSubscriptionFragment.class.getSimpleName();
  private final static String EXTRA_CURRENT_STATE = "extra_current_state";
  private final static String EXTRA_PRODUCT_DETAILS = "extra_product_details";

  private boolean mPingingResult;
  private boolean mValidationResult;
  @NonNull
  private final PingCallback mPingCallback = new PingCallback();
  @NonNull
  private final BookmarkSubscriptionCallback mPurchaseCallback = new BookmarkSubscriptionCallback();
  @NonNull
  private BookmarkSubscriptionPaymentState mState = BookmarkSubscriptionPaymentState.NONE;
  @Nullable
  private ProductDetails[] mProductDetails;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private PurchaseController<PurchaseCallback> mPurchaseController;

  @Nullable
  @Override
  public final View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                                 @Nullable Bundle savedInstanceState)
  {
    mPurchaseController =
        PurchaseFactory.createBookmarksSubscriptionPurchaseController(requireContext());
    if (savedInstanceState != null)
      mPurchaseController.onRestore(savedInstanceState);
    mPurchaseController.initialize(requireActivity());
    mPingCallback.attach(this);
    BookmarkManager.INSTANCE.addCatalogPingListener(mPingCallback);
    return onSubscriptionCreateView(inflater, container, savedInstanceState);
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

  @Override
  public final void onDestroyView()
  {
    super.onDestroyView();
    mPingCallback.detach();
    BookmarkManager.INSTANCE.removeCatalogPingListener(mPingCallback);
    onSubscriptionDestroyView();
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

  void launchPurchaseFlow(@NonNull String productId)
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

    onAuthorizationFinishSuccessfully();
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
    if (ConnectionState.isConnected())
      NetworkPolicy.checkNetworkPolicy(requireFragmentManager(),
                                       this::onNetworkPolicyResult, true);
    else
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

  private void onNetworkPolicyResult(@NonNull NetworkPolicy policy)
  {
    if (policy.canUseNetwork())
      onNetworkCheckPassed();
    else
      requireActivity().finish();
  }

  private void onNetworkCheckPassed()
  {
    activateState(BookmarkSubscriptionPaymentState.PRODUCT_DETAILS_LOADING);
  }

  @Override
  @CallSuper
  public void onProductDetailsLoading()
  {
    queryProductDetails();
  }

  @Override
  @CallSuper
  public void onPingFinish()
  {
    finishPinging();
  }

  @Override
  @CallSuper
  public void onValidationFinish()
  {
    finishValidation();
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

  @Nullable
  abstract View onSubscriptionCreateView(@NonNull LayoutInflater inflater,
                                         @Nullable ViewGroup container,
                                         @Nullable Bundle savedInstanceState);

  abstract void onSubscriptionDestroyView();

  abstract void onAuthorizationFinishSuccessfully();


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
