package com.mapswithme.maps.purchase;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.billingclient.api.BillingClient;
import com.android.billingclient.api.SkuDetails;
import com.mapswithme.maps.base.Detachable;
import com.mapswithme.util.statistics.Statistics;

import java.util.Collections;
import java.util.List;

class BookmarkPurchaseCallback
    extends StatefulPurchaseCallback<BookmarkPaymentState, BookmarkPaymentFragment>
    implements PurchaseCallback, Detachable<BookmarkPaymentFragment>, CoreStartTransactionObserver
{
  @Nullable
  private List<SkuDetails> mPendingDetails;
  private Boolean mPendingValidationResult;
  @NonNull
  private final String mServerId;

  BookmarkPurchaseCallback(@NonNull String serverId)
  {
    mServerId = serverId;
  }

  @Override
  public void onStartTransaction(boolean success, @NonNull String serverId, @NonNull String
      vendorId)
  {
    if (!success)
    {
      activateStateSafely(BookmarkPaymentState.TRANSACTION_FAILURE);
      return;
    }

    activateStateSafely(BookmarkPaymentState.TRANSACTION_STARTED);
  }

  @Override
  public void onProductDetailsLoaded(@NonNull List<SkuDetails> details)
  {
    if (getUiObject() == null)
      mPendingDetails = Collections.unmodifiableList(details);
    else
      getUiObject().handleProductDetails(details);
    activateStateSafely(BookmarkPaymentState.PRODUCT_DETAILS_LOADED);
  }

  @Override
  public void onPaymentFailure(@BillingClient.BillingResponse int error)
  {
    activateStateSafely(BookmarkPaymentState.PAYMENT_FAILURE);
  }

  @Override
  public void onProductDetailsFailure()
  {
    activateStateSafely(BookmarkPaymentState.PRODUCT_DETAILS_FAILURE);
  }

  @Override
  public void onStoreConnectionFailed()
  {
    activateStateSafely(BookmarkPaymentState.PRODUCT_DETAILS_FAILURE);
  }

  @Override
  public void onValidationStarted()
  {
    Statistics.INSTANCE.trackPurchaseEvent(Statistics.EventName.INAPP_PURCHASE_STORE_SUCCESS,
                                           mServerId);
    activateStateSafely(BookmarkPaymentState.VALIDATION);
  }

  @Override
  public void onValidationFinish(boolean success)
  {
    if (getUiObject() == null)
      mPendingValidationResult = success;
    else
      getUiObject().handleValidationResult(success);

    activateStateSafely(BookmarkPaymentState.VALIDATION_FINISH);
  }

  @Override
  void onAttach(@NonNull BookmarkPaymentFragment uiObject)
  {
    if (mPendingDetails != null)
    {
      uiObject.handleProductDetails(mPendingDetails);
      mPendingDetails = null;
    }

    if (mPendingValidationResult != null)
    {
      uiObject.handleValidationResult(mPendingValidationResult);
      mPendingValidationResult = null;
    }
  }
}
