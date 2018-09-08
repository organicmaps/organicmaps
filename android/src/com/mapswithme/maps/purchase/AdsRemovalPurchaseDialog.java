package com.mapswithme.maps.purchase;

import android.content.Context;
import android.support.annotation.Nullable;

import com.mapswithme.maps.base.BaseMwmDialogFragment;

public class AdsRemovalPurchaseDialog extends BaseMwmDialogFragment
{
  @Nullable
  private PurchaseController<AdsRemovalPurchaseCallback> mController;

  @Override
  public void onAttach(Context context)
  {
    super.onAttach(context);
    mController = ((AdsRemovalPurchaseControllerProvider) context).getAdsRemovalPurchaseController();
  }

  @Override
  public void onDetach()
  {
    super.onDetach();
    mController = null;
  }

  // UI implementation is coming soon.
}
