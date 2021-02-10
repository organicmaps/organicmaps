package com.mapswithme.maps.purchase;




public abstract class AbstractProductDetailsLoadingCallback implements PlayStoreBillingCallback
{

  public abstract void onProductDetailsFailure();

  @Override
  public void onStoreConnectionFailed()
  {
    // Do nothing.
  }



  @Override
  public void onConsumptionSuccess()
  {
    // Do nothing.
  }

  @Override
  public void onConsumptionFailure()
  {
    // Do nothing.
  }
}
