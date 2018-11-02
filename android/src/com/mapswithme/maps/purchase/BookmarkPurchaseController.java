package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.support.annotation.NonNull;

import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.SkuDetails;

import java.util.List;

class BookmarkPurchaseController extends AbstractPurchaseController<ValidationCallback,
    PlayStoreBillingCallback, BookmarkPurchaseCallback>
{
  @NonNull
  private final PlayStoreBillingCallback mBillingCallback = new PlayStoreBillingCallbackImpl();

  BookmarkPurchaseController(@NonNull PurchaseValidator<ValidationCallback> validator,
                             @NonNull BillingManager<PlayStoreBillingCallback> billingManager,
                             @NonNull String... productIds)
  {
    super(validator, billingManager, productIds);
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
    // TODO: coming soon.
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
