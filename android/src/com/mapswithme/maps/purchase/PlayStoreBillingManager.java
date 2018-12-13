package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.BillingClient.BillingResponse;
import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.PurchasesUpdatedListener;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.ArrayList;
import java.util.List;

class PlayStoreBillingManager implements BillingManager<PlayStoreBillingCallback>,
                                                PurchasesUpdatedListener,
                                                PlayStoreBillingConnection.ConnectionListener
{
  final static Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  final static String TAG = PlayStoreBillingManager.class.getSimpleName();
  @Nullable
  private Activity mActivity;
  @Nullable
  private BillingClient mBillingClient;
  @Nullable
  private PlayStoreBillingCallback mCallback;
  @NonNull
  @BillingClient.SkuType
  private final String mProductType;
  @SuppressWarnings({ "NullableProblems" })
  @NonNull
  private BillingConnection mConnection;
  @NonNull
  private final List<BillingRequest> mPendingRequests = new ArrayList<>();
  
  PlayStoreBillingManager(@NonNull @BillingClient.SkuType String productType)
  {
    mProductType = productType;
  }

  @Override
  public void initialize(@NonNull Activity context)
  {
    LOGGER.i(TAG, "Creating play store billing client...");
    mActivity = context;
    mBillingClient = BillingClient.newBuilder(mActivity).setListener(this).build();
    mConnection = new PlayStoreBillingConnection(mBillingClient, this);
    mConnection.open();
  }

  @Override
  public void destroy()
  {
    mActivity = null;
    mConnection.close();
    mPendingRequests.clear();
  }

  @Override
  public void queryProductDetails(@NonNull List<String> productIds)
  {
    executeBillingRequest(new QueryProductDetailsRequest(getClientOrThrow(), mProductType,
                                                         mCallback, productIds));
  }

  @Override
  public void queryExistingPurchases()
  {
    executeBillingRequest(new QueryExistingPurchases(getClientOrThrow(), mProductType, mCallback));
  }

  @Override
  public void consumePurchase(@NonNull String purchaseToken)
  {
    executeBillingRequest(new ConsumePurchaseRequest(getClientOrThrow(), mProductType, mCallback,
                                                     purchaseToken));
  }

  private void executeBillingRequest(@NonNull BillingRequest request)
  {
    switch (mConnection.getState())
    {
      case CONNECTING:
        mPendingRequests.add(request);
        break;
      case CONNECTED:
        request.execute();
        break;
      case DISCONNECTED:
        mPendingRequests.add(request);
        mConnection.open();
        break;
      case CLOSED:
        throw new IllegalStateException("Billing service connection already closed, " +
                                        "please initialize it again.");
    }
  }

  @Override
  public void launchBillingFlowForProduct(@NonNull String productId)
  {
    if (!isBillingSupported())
    {
      LOGGER.w(TAG, "Purchase type '" + mProductType + "' is not supported by this device!");
      return;
    }

    executeBillingRequest(new LaunchBillingFlowRequest(getActivityOrThrow(), getClientOrThrow(),
                                                       mProductType, productId));
  }

  @Override
  public boolean isBillingSupported()
  {
    if (BillingClient.SkuType.SUBS.equals(mProductType))
    {
      @BillingResponse
      int result = getClientOrThrow().isFeatureSupported(BillingClient.FeatureType.SUBSCRIPTIONS);
      return result != BillingResponse.FEATURE_NOT_SUPPORTED;
    }

    return true;
  }

  @Override
  public void addCallback(@NonNull PlayStoreBillingCallback callback)
  {
    mCallback = callback;
  }

  @Override
  public void removeCallback(@NonNull PlayStoreBillingCallback callback)
  {
    mCallback = null;
  }

  @Override
  public void onPurchasesUpdated(int responseCode, @Nullable List<Purchase> purchases)
  {
    if (responseCode == BillingResponse.USER_CANCELED)
    {
      LOGGER.i(TAG, "Billing cancelled by user.");
      return;
    }

    if (responseCode == BillingResponse.ITEM_ALREADY_OWNED)
    {
      LOGGER.i(TAG, "Billing already done before.");
      return;
    }

    if (responseCode != BillingResponse.OK || purchases == null || purchases.isEmpty())
    {
      LOGGER.e(TAG, "Billing failed. Response code: " + responseCode);
      if (mCallback != null)
        mCallback.onPurchaseFailure(responseCode);
      return;
    }

    LOGGER.i(TAG, "Purchase process successful. Count of purchases: " + purchases.size());
    if (mCallback != null)
      mCallback.onPurchaseSuccessful(purchases);
  }

  @NonNull
  private BillingClient getClientOrThrow()
  {
    if (mBillingClient == null)
      throw new IllegalStateException("Manager must be initialized! Call 'initialize' method first.");

    return mBillingClient;
  }

  @NonNull
  private Activity getActivityOrThrow()
  {
    if (mActivity == null)
      throw new IllegalStateException("Manager must be initialized! Call 'initialize' method first.");

    return mActivity;
  }

  @Override
  public void onConnected()
  {
    for(@NonNull BillingRequest request: mPendingRequests)
      request.execute();

    mPendingRequests.clear();
  }

  @Override
  public void onDisconnected()
  {
    LOGGER.w(TAG, "Play store connection failed.");
    if (mPendingRequests.isEmpty())
      return;

    mPendingRequests.clear();
    if (mCallback != null)
      mCallback.onStoreConnectionFailed();
  }
}
