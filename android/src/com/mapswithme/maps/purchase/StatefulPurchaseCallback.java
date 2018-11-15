package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.base.Detachable;

abstract class StatefulPurchaseCallback<State, UiObject extends PurchaseStateActivator<State>>
    implements Detachable<UiObject>
{
  @Nullable
  private State mPendingState;
  @Nullable
  private UiObject mUiObject;

  void activateStateSafely(@NonNull State state)
  {
    if (mUiObject == null)
    {
      mPendingState = state;
      return;
    }

    mUiObject.activateState(state);
  }

  @Override
  public final void attach(@NonNull UiObject uiObject)
  {
    mUiObject = uiObject;
    if (mPendingState != null)
    {
      mUiObject.activateState(mPendingState);
      mPendingState = null;
    }
    onAttach(uiObject);
  }

  @Override
  public final void detach()
  {
    onDetach();
    mUiObject = null;
  }

  @Nullable
  UiObject getUiObject()
  {
    return mUiObject;
  }

  void onAttach(@NonNull UiObject uiObject)
  {
    // Do nothing by default.
  }

  void onDetach()
  {
    // Do nothing by default.
  }
}
