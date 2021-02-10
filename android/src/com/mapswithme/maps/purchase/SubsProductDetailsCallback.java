package com.mapswithme.maps.purchase;


class SubsProductDetailsCallback
    extends StatefulPurchaseCallback<BookmarkPaymentState, BookmarkPaymentFragment>
    implements PlayStoreBillingCallback
{




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
