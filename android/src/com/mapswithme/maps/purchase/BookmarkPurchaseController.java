package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.SkuDetails;

import java.util.List;

class BookmarkPurchaseController extends AbstractPurchaseController<ValidationCallback,
    PlayStoreBillingCallback, BookmarkPurchaseCallback>
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

  private class PlayStoreBillingCallbackImpl implements PlayStoreBillingCallback
  {

    @Override
    public void onPurchaseDetailsLoaded(@NonNull List<SkuDetails> details)
    {

    }

    @Override
    public void onPurchaseSuccessful(@NonNull List<Purchase> purchases)
    {
      Purchase target = findTargetPurchase(purchases);
      if (target == null)
        return;

      // TODO: use vendor from private.h
      getValidator().validate(mServerId, "bookmarks", target.getOriginalJson());
    }

    @Override
    public void onPurchaseFailure(int error)
    {

    }

    @Override
    public void onPurchaseDetailsFailure()
    {

    }

    @Override
    public void onStoreConnectionFailed()
    {

    }

    @Override
    public void onPurchasesLoaded(@NonNull List<Purchase> purchases)
    {

    }
  }
}
