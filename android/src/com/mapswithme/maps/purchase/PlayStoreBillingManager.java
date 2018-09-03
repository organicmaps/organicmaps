package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.BillingClient.BillingResponse;
import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.PurchasesUpdatedListener;
import com.android.billingclient.api.SkuDetails;
import com.android.billingclient.api.SkuDetailsParams;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class PlayStoreBillingManager implements BillingManager<PlayStoreBillingCallback>,
                                                PurchasesUpdatedListener,
                                                PlayStoreBillingConnection.ConnectionListener
{
  private final static Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private final static String TAG = PlayStoreBillingManager.class.getSimpleName();
  @NonNull
  private final Activity mActivity;
  @Nullable
  private BillingClient mBillingClient;
  @Nullable
  private PlayStoreBillingCallback mCallback;
  @NonNull
  private final String mProductId;
  @NonNull
  @BillingClient.SkuType
  private final String mProductType;
  @SuppressWarnings({ "NullableProblems" })
  @NonNull
  private BillingConnection mConnection;
  @NonNull
  private final List<Runnable> mPendingRequests = new ArrayList<>();
  
  PlayStoreBillingManager(@NonNull Activity activity, @NonNull String productId,
                          @NonNull @BillingClient.SkuType String productType)
  {
    mActivity = activity;
    mProductId = productId;
    mProductType = productType;
  }

  @Override
  public void initialize()
  {
    LOGGER.i(TAG, "Creating play store billing client...");
    mBillingClient = BillingClient.newBuilder(mActivity).setListener(this).build();
    mConnection = new PlayStoreBillingConnection(mBillingClient, this);
    mConnection.connect();
  }

  @Override
  public void destroy()
  {
    // Coming soon.
  }

  @Override
  public void queryProductDetails()
  {
    executeServiceRequest(new QueryProductDetailsRequest());
  }

  private void executeServiceRequest(@NonNull Runnable task)
  {
    switch (mConnection.getState())
    {
      case CONNECTING:
        mPendingRequests.add(task);
        break;
      case CONNECTED:
        task.run();
        break;
      case DISCONNECTED:
        mConnection.connect();
        break;
      case CLOSED:
        throw new IllegalStateException("Billing service connection already closed, " +
                                        "please initialize it again.");
    }
  }

  @Override
  public void launchBillingFlow()
  {
    @BillingResponse
    int result = getClientOrThrow().isFeatureSupported(BillingClient.FeatureType.SUBSCRIPTIONS);
    if (result == BillingResponse.FEATURE_NOT_SUPPORTED)
    {
      LOGGER.w(TAG, "Subscription is not supported by this device!");
      return;
    }

    // Coming soon.
  }

  @Nullable
  @Override
  public String obtainPurchaseToken()
  {
    return null;
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
    // Coming soon.
  }

  @NonNull
  private BillingClient getClientOrThrow()
  {
    if (mBillingClient == null)
      throw new IllegalStateException("Manager must be initialized! Call 'initialize' method first.");

    return mBillingClient;
  }

  @Override
  public void onConnected()
  {
    for(@NonNull Runnable request: mPendingRequests)
      request.run();

    mPendingRequests.clear();
  }

  private class QueryProductDetailsRequest implements Runnable
  {
    @Override
    public void run()
    {
      SkuDetailsParams.Builder builder = SkuDetailsParams.newBuilder();
      builder.setSkusList(Collections.singletonList(mProductId))
             .setType(mProductType);
      getClientOrThrow().querySkuDetailsAsync(builder.build(), this::onSkuDetailsResponseInternal);
    }

    private void onSkuDetailsResponseInternal(@BillingResponse int responseCode,
                                              @Nullable List<SkuDetails> skuDetails)
    {
      LOGGER.i(TAG, "Product details response, code: " + responseCode + ", " +
                    "details: " + skuDetails);
    }
  }
}
