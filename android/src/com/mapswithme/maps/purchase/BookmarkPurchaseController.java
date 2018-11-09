package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.SkuDetails;
import com.mapswithme.maps.PrivateVariables;

import java.util.List;

class BookmarkPurchaseController extends AbstractPurchaseController<ValidationCallback,
    PlayStoreBillingCallback, PurchaseCallback>
{
  @NonNull
  private final PlayStoreBillingCallback mBillingCallback = new PlayStoreBillingCallbackImpl();
  @Nullable
  private final String mServerId;

  BookmarkPurchaseController(@NonNull PurchaseValidator<ValidationCallback> validator,
                             @NonNull BillingManager<PlayStoreBillingCallback> billingManager,
                             @NonNull String productId, @Nullable String serverId)
  {
    super(validator, billingManager, productId);
    mServerId = serverId;
  }

  @Override
  void onInitialize(@NonNull Activity activity)
  {
    getBillingManager().addCallback(mBillingCallback);
    // TODO: coming soon.
  }

  @Override
  void onDestroy()
  {
    getBillingManager().removeCallback(mBillingCallback);
    // TODO: coming soon.
  }

  @Override
  public void queryPurchaseDetails()
  {
//    getBillingManager().queryProductDetails(mProductIds);
    // TODO: just for testing.
    getBillingManager().queryExistingPurchases();
  }

  private class PlayStoreBillingCallbackImpl extends AbstractPlayStoreBillingCallback
  {

    @Override
    public void onPurchaseDetailsLoaded(@NonNull List<SkuDetails> details)
    {
      if (getUiCallback() != null)
        getUiCallback().onProductDetailsFailure();
    }

    @Override
    void validate(@NonNull String purchaseData)
    {
      getValidator().validate(mServerId, PrivateVariables.bookmarksVendor(), purchaseData);
    }

    @Override
    public void onPurchasesLoaded(@NonNull List<Purchase> purchases)
    {
      // TODO: coming soon.
    }
  }
}
