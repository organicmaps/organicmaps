package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.android.billingclient.api.SkuDetails;
import com.bumptech.glide.Glide;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.PrivateVariables;
import com.mapswithme.maps.PurchaseOperationObservable;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.bookmarks.data.PaymentData;
import com.mapswithme.maps.dialog.AlertDialogCallback;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;

import java.util.Collections;
import java.util.List;

public class BookmarkPaymentFragment extends BaseMwmFragment
    implements AlertDialogCallback, PurchaseStateActivator<BookmarkPaymentState>
{
  static final String ARG_PAYMENT_DATA = "arg_payment_data";
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private static final String TAG = BookmarkPaymentFragment.class.getSimpleName();
  private static final String EXTRA_CURRENT_STATE = "extra_current_state";
  private static final String EXTRA_PRODUCT_DETAILS = "extra_product_details";
  private static final String EXTRA_SUBS_PRODUCT_DETAILS = "extra_subs_product_details";
  private static final String EXTRA_VALIDATION_RESULT = "extra_validation_result";

  @SuppressWarnings("NullableProblems")
  @NonNull
  private PurchaseController<PurchaseCallback> mPurchaseController;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private BookmarkPurchaseCallback mPurchaseCallback;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private PaymentData mPaymentData;
  @Nullable
  private ProductDetails mProductDetails;
  @Nullable
  private ProductDetails mSubsProductDetails;
  private boolean mValidationResult;
  @NonNull
  private BookmarkPaymentState mState = BookmarkPaymentState.NONE;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private BillingManager<PlayStoreBillingCallback> mSubsProductDetailsLoadingManager;

  @NonNull
  private final SubsProductDetailsCallback mSubsProductDetailsCallback
      = new SubsProductDetailsCallback();

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    Bundle args = getArguments();
    if (args == null)
      throw new IllegalStateException("Args must be provided for payment fragment!");

    PaymentData paymentData = args.getParcelable(ARG_PAYMENT_DATA);
    if (paymentData == null)
      throw new IllegalStateException("Payment data must be provided for payment fragment!");

    mPaymentData = paymentData;
    mPurchaseCallback = new BookmarkPurchaseCallback(mPaymentData.getServerId());
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable
      Bundle savedInstanceState)
  {
    mPurchaseController = PurchaseFactory.createBookmarkPurchaseController(requireContext(),
                                                                           mPaymentData.getProductId(),
                                                                           mPaymentData.getServerId());
    if (savedInstanceState != null)
      mPurchaseController.onRestore(savedInstanceState);

    mPurchaseController.initialize(requireActivity());

    mSubsProductDetailsLoadingManager = PurchaseFactory.createSubscriptionBillingManager();
    mSubsProductDetailsLoadingManager.initialize(requireActivity());
    mSubsProductDetailsLoadingManager.addCallback(mSubsProductDetailsCallback);
    mSubsProductDetailsCallback.attach(this);

    View root = inflater.inflate(R.layout.fragment_bookmark_payment, container, false);
    View subscriptionButton = root.findViewById(R.id.buy_subs_btn);
    subscriptionButton.setOnClickListener(v -> onBuySubscriptionClicked());
    TextView buyInappBtn = root.findViewById(R.id.buy_inapp_btn);
    buyInappBtn.setOnClickListener(v -> onBuyInappClicked());
    return root;
  }

  private void onBuySubscriptionClicked()
  {
    SubscriptionType type = SubscriptionType.getTypeByBookmarksGroup(mPaymentData.getGroup());

    if (type.equals(SubscriptionType.BOOKMARKS_SIGHTS))
    {
      BookmarksSightsSubscriptionActivity.startForResult
          (this, PurchaseUtils.REQ_CODE_PAY_SUBSCRIPTION, Statistics.ParamValue.CARD);
      return;
    }

    BookmarksAllSubscriptionActivity.startForResult
        (this, PurchaseUtils.REQ_CODE_PAY_SUBSCRIPTION, Statistics.ParamValue.CARD);
  }

  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);
    if (resultCode != Activity.RESULT_OK)
      return;

    if (requestCode == PurchaseUtils.REQ_CODE_PAY_SUBSCRIPTION)
    {
      Intent intent = new Intent();
      intent.putExtra(PurchaseUtils.EXTRA_IS_SUBSCRIPTION, true);
      requireActivity().setResult(Activity.RESULT_OK, intent);
      requireActivity().finish();
    }
  }

  private void onBuyInappClicked()
  {
    Statistics.INSTANCE.trackPurchasePreviewSelect(mPaymentData.getServerId(),
                                                   mPaymentData.getProductId());
    Statistics.INSTANCE.trackPurchaseEvent(Statistics.EventName.INAPP_PURCHASE_PREVIEW_PAY,
                                           mPaymentData.getServerId(),
                                           Statistics.STATISTICS_CHANNEL_REALTIME);
    startPurchaseTransaction();
  }

  @Override
  public boolean onBackPressed()
  {
    if (mState == BookmarkPaymentState.VALIDATION)
    {
      Utils.showSnackbar(requireContext(), getView(),
                         R.string.purchase_please_wait_toast);
      return true;
    }

    Statistics.INSTANCE.trackPurchaseEvent(Statistics.EventName.INAPP_PURCHASE_PREVIEW_CANCEL,
                                           mPaymentData.getServerId());
    return super.onBackPressed();
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    if (savedInstanceState == null)
      Statistics.INSTANCE.trackPurchasePreviewShow(mPaymentData.getServerId(),
                                                   PrivateVariables.bookmarksVendor(),
                                                   mPaymentData.getProductId());
    LOGGER.d(TAG, "onViewCreated savedInstanceState = " + savedInstanceState);
    setInitialPaymentData();
    loadImage();
    if (savedInstanceState != null)
    {
      mProductDetails = savedInstanceState.getParcelable(EXTRA_PRODUCT_DETAILS);
      if (mProductDetails != null)
        updateProductDetails();
      mSubsProductDetails = savedInstanceState.getParcelable(EXTRA_SUBS_PRODUCT_DETAILS);
      if (mSubsProductDetails != null)
        updateSubsProductDetails();
      mValidationResult = savedInstanceState.getBoolean(EXTRA_VALIDATION_RESULT);
      BookmarkPaymentState savedState
          = BookmarkPaymentState.values()[savedInstanceState.getInt(EXTRA_CURRENT_STATE)];
      activateState(savedState);
      return;
    }

    activateState(BookmarkPaymentState.PRODUCT_DETAILS_LOADING);
    mPurchaseController.queryProductDetails();
    SubscriptionType type = SubscriptionType.getTypeByBookmarksGroup(mPaymentData.getGroup());
    List<String> subsProductIds =
        Collections.singletonList(type.getMonthlyProductId());
    mSubsProductDetailsLoadingManager.queryProductDetails(subsProductIds);
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();
    mPurchaseController.destroy();
    mSubsProductDetailsLoadingManager.removeCallback(mSubsProductDetailsCallback);
    mSubsProductDetailsCallback.detach();
    mSubsProductDetailsLoadingManager.destroy();
  }

  private void startPurchaseTransaction()
  {
    activateState(BookmarkPaymentState.TRANSACTION_STARTING);
    Framework.nativeStartPurchaseTransaction(mPaymentData.getServerId(),
                                             PrivateVariables.bookmarksVendor());
  }

  void launchBillingFlow()
  {
    mPurchaseController.launchPurchaseFlow(mPaymentData.getProductId());
    activateState(BookmarkPaymentState.PAYMENT_IN_PROGRESS);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    PurchaseOperationObservable observable = PurchaseOperationObservable.from(requireContext());
    observable.addTransactionObserver(mPurchaseCallback);
    mPurchaseController.addCallback(mPurchaseCallback);
    mPurchaseCallback.attach(this);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    PurchaseOperationObservable observable = PurchaseOperationObservable.from(requireContext());
    observable.removeTransactionObserver(mPurchaseCallback);
    mPurchaseController.removeCallback();
    mPurchaseCallback.detach();
  }

  @Override
  public void onSaveInstanceState(Bundle outState)
  {
    super.onSaveInstanceState(outState);
    LOGGER.d(TAG, "onSaveInstanceState");
    outState.putInt(EXTRA_CURRENT_STATE, mState.ordinal());
    outState.putParcelable(EXTRA_PRODUCT_DETAILS, mProductDetails);
    outState.putParcelable(EXTRA_SUBS_PRODUCT_DETAILS, mSubsProductDetails);
    mPurchaseController.onSave(outState);
  }

  @Override
  public void activateState(@NonNull BookmarkPaymentState state)
  {
    if (state == mState)
      return;

    LOGGER.i(TAG, "Activate state: " + state);
    mState = state;
    mState.activate(this);
  }

  private void loadImage()
  {
    if (TextUtils.isEmpty(mPaymentData.getImgUrl()))
      return;

    ImageView imageView = getViewOrThrow().findViewById(R.id.image);
    Glide.with(imageView.getContext())
         .load(mPaymentData.getImgUrl())
         .centerCrop()
         .into(imageView);
  }

  private void setInitialPaymentData()
  {
    TextView name = getViewOrThrow().findViewById(R.id.product_catalog_name);
    name.setText(mPaymentData.getName());
    TextView author = getViewOrThrow().findViewById(R.id.author_name);
    author.setText(mPaymentData.getAuthorName());
  }

  void handleProductDetails(@NonNull List<SkuDetails> details)
  {
    if (details.isEmpty())
      return;

    SkuDetails skuDetails = details.get(0);
    mProductDetails = PurchaseUtils.toProductDetails(skuDetails);
  }

  void handleSubsProductDetails(@NonNull List<SkuDetails> details)
  {
    if (details.isEmpty())
      return;

    SkuDetails skuDetails = details.get(0);
    mSubsProductDetails = PurchaseUtils.toProductDetails(skuDetails);
  }

  void handleValidationResult(boolean validationResult)
  {
    mValidationResult = validationResult;
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
      case PurchaseUtils.REQ_CODE_PRODUCT_DETAILS_FAILURE:
        requireActivity().finish();
        break;
      case PurchaseUtils.REQ_CODE_START_TRANSACTION_FAILURE:
      case PurchaseUtils.REQ_CODE_PAYMENT_FAILURE:
        activateState(BookmarkPaymentState.PRODUCT_DETAILS_LOADED);
        break;
    }
  }

  void updateProductDetails()
  {
    if (mProductDetails == null)
      throw new AssertionError("Product details must be obtained at this moment!");

    TextView buyButton = getViewOrThrow().findViewById(R.id.buy_inapp_btn);
    String price = Utils.formatCurrencyString(mProductDetails.getPrice(),
                                              mProductDetails.getCurrencyCode());
    buyButton.setText(getString(R.string.buy_btn, price));
    TextView storeName = getViewOrThrow().findViewById(R.id.product_store_name);
    storeName.setText(mProductDetails.getTitle());
  }

  void updateSubsProductDetails()
  {
    if (mSubsProductDetails == null)
      throw new AssertionError("Subs product details must be obtained at this moment!");

    String formattedPrice = Utils.formatCurrencyString(mSubsProductDetails.getPrice(),
                                                       mSubsProductDetails.getCurrencyCode());
    TextView subsButton = getViewOrThrow().findViewById(R.id.buy_subs_btn);
    subsButton.setText(getString(R.string.buy_btn_for_subscription_version_2, formattedPrice));
  }

  void finishValidation()
  {
    if (mValidationResult)
      requireActivity().setResult(Activity.RESULT_OK);

    requireActivity().finish();
  }
}
