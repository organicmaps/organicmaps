package com.mapswithme.maps.purchase;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.dialog.Detachable;

abstract class StatefulPurchaseCallback<State, UiContext extends PurchaseStateActivator<State>>
    implements Detachable<UiContext>
{
  @Nullable
  private State mPendingState;
  @Nullable
  private UiContext mUiContext;

  void activateStateSafely(@NonNull State state)
  {
    if (mUiContext == null)
    {
      mPendingState = state;
      return;
    }

    mUiContext.activateState(state);
  }

  @Override
  public final void attach(@NonNull UiContext context)
  {
    mUiContext = context;
    if (mPendingState != null)
    {
      mUiContext.activateState(mPendingState);
      mPendingState = null;
    }
    onAttach(context);
  }

  @Override
  public final void detach()
  {
    onDetach();
    mUiContext = null;
  }

  @Nullable
  UiContext getUiContext()
  {
    return mUiContext;
  }

  void onAttach(@NonNull UiContext context)
  {
    // Do nothing by default.
  }

  void onDetach()
  {
    // Do nothing by default.
  }
}
