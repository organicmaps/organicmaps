package com.mapswithme.maps.purchase;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.base.BaseAuthFragment;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

abstract class AbstractBookmarkSubscriptionFragment extends BaseAuthFragment
    implements PurchaseStateActivator<BookmarkSubscriptionPaymentState>, SubscriptionUiChangeListener
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private static final String TAG = AbstractBookmarkSubscriptionFragment.class.getSimpleName();

  private boolean mPingingResult;
  @NonNull
  private final PingCallback mPingCallback = new PingCallback();

  @Nullable
  @Override
  public final View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    mPingCallback.attach(this);
    BookmarkManager.INSTANCE.addCatalogPingListener(mPingCallback);
    return onSubscriptionCreateView(inflater, container, savedInstanceState);
  }

  @Override
  public final void onDestroyView()
  {
    super.onDestroyView();
    mPingCallback.detach();
    BookmarkManager.INSTANCE.removeCatalogPingListener(mPingCallback);
    onSubscriptionDestroyView();
  }

  @Nullable
  abstract View onSubscriptionCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                                         @Nullable Bundle savedInstanceState);

  abstract void onSubscriptionDestroyView();

  private void handlePingingResult(boolean result)
  {
    mPingingResult = result;
  }

  private void finishPinging()
  {
    if (!mPingingResult)
    {
      PurchaseUtils.showPingFailureDialog(this);
      return;
    }

    authorize();
  }

  @Override
  @CallSuper
  public void onPingFinish()
  {
    finishPinging();
  }

  private static class PingCallback
      extends StatefulPurchaseCallback<BookmarkSubscriptionPaymentState,
      AbstractBookmarkSubscriptionFragment> implements BookmarkManager.BookmarksCatalogPingListener

  {
    private Boolean mPendingPingingResult;

    @Override
    public void onPingFinished(boolean isServiceAvailable)
    {
      LOGGER.i(TAG, "Ping finished, isServiceAvailable: " + isServiceAvailable);
      if (getUiObject() == null)
        mPendingPingingResult = isServiceAvailable;
      else
        getUiObject().handlePingingResult(isServiceAvailable);

      activateStateSafely(BookmarkSubscriptionPaymentState.PINGING_FINISH);
    }

    @Override
    void onAttach(@NonNull AbstractBookmarkSubscriptionFragment fragment)
    {
      if (mPendingPingingResult != null)
      {
        fragment.handlePingingResult(mPendingPingingResult);
        mPendingPingingResult = null;
      }
    }
  }
}
