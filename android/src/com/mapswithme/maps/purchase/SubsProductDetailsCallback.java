package com.mapswithme.maps.purchase;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.billingclient.api.Purchase;
import com.android.billingclient.api.SkuDetails;

import java.util.Collections;
import java.util.List;

class SubsProductDetailsCallback
    extends StatefulPurchaseCallback<BookmarkPaymentState, BookmarkPaymentFragment>
    implements PlayStoreBillingCallback
{
  @Nullable
  private List<SkuDetails> mPendingDetails;

  @Override
  public void onProductDetailsLoaded(@NonNull List<SkuDetails> details)
  {
    if (PurchaseUtils.hasIncorrectSkuDetails(details))
    {
      activateStateSafely(BookmarkPaymentState.SUBS_PRODUCT_DETAILS_FAILURE);
      return;
    }

    if (getUiObject() == null)
      mPendingDetails = Collections.unmodifiableList(details);
    else
      getUiObject().handleSubsProductDetails(details);

    activateStateSafely(BookmarkPaymentState.SUBS_PRODUCT_DETAILS_LOADED);
  }


  @Override
  void onAttach(@NonNull BookmarkPaymentFragment bookmarkPaymentFragment)
  {
    if (mPendingDetails != null)
    {
      bookmarkPaymentFragment.handleProductDetails(mPendingDetails);
      mPendingDetails = null;
    }
  }

  @Override
  public void onPurchaseSuccessful(@NonNull List<Purchase> purchases)
  {
    // Do nothing.
  }

  @Override
  public void onPurchaseFailure(int error)
  {
    // Do nothing.
  }

  @Override
  public void onProductDetailsFailure()
  {
    activateStateSafely(BookmarkPaymentState.SUBS_PRODUCT_DETAILS_FAILURE);
  }

  @Override
  public void onStoreConnectionFailed()
  {
    // Do nothing.
  }

  @Override
  public void onPurchasesLoaded(@NonNull List<Purchase> purchases)
  {
    // Do nothing.
  }

  @Override
  public void onConsumptionSuccess()
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public void onConsumptionFailure()
  {
    throw new UnsupportedOperationException();
  }
}
