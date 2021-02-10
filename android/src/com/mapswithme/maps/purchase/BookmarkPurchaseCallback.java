package com.mapswithme.maps.purchase;

import androidx.annotation.NonNull;

import com.mapswithme.maps.base.Detachable;


class BookmarkPurchaseCallback
    extends StatefulPurchaseCallback<BookmarkPaymentState, BookmarkPaymentFragment>
    implements  Detachable<BookmarkPaymentFragment>, CoreStartTransactionObserver
{

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
  void onAttach(@NonNull BookmarkPaymentFragment uiObject)
  {

    if (mPendingValidationResult != null)
    {
      uiObject.handleValidationResult(mPendingValidationResult);
      mPendingValidationResult = null;
    }
  }
}
