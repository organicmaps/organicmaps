package com.mapswithme.maps.purchase;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.PrivateVariables;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.bookmarks.data.PaymentData;
import com.mapswithme.maps.dialog.Detachable;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

public class BookmarkPaymentFragment extends BaseMwmFragment
    implements PurchaseStateActivator<BookmarkPaymentState>
{
  static final String ARG_PAYMENT_DATA = "arg_payment_data";
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = BookmarkPaymentFragment.class.getSimpleName();
  private static final String EXTRA_CURRENT_STATE = "extra_current_state";

  @SuppressWarnings("NullableProblems")
  @NonNull
  private PurchaseController<BookmarkPurchaseCallback> mPurchaseController;
  @NonNull
  private BookmarkPurchaseCallbackImpl mPurchaseCallback = new BookmarkPurchaseCallbackImpl();
  @SuppressWarnings("NullableProblems")
  @NonNull
  private PaymentData mPaymentData;
  @NonNull
  private BookmarkPaymentState mState = BookmarkPaymentState.NONE;

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    Bundle args = getArguments();
    if (args == null)
      throw new IllegalStateException("Args must be provided for payment fragment!");

    PaymentData paymentData = args.getParcelable(ARG_PAYMENT_DATA);
    if (paymentData == null)
      throw new IllegalStateException("Payment data must be provided for payment fragment!");

    mPaymentData = paymentData;
    mPurchaseController = PurchaseFactory.createBookmarkPurchaseController(mPaymentData.getProductId(),
                                                                           mPaymentData.getServerId());
    mPurchaseController.initialize(getActivity());
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable
      Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_bookmark_payment, container, false);
    // TODO: temporary launch of billing flow.
    root.findViewById(R.id.buy_inapp).setOnClickListener(v -> startPurchaseTransaction());
    root.findViewById(R.id.query_inapps).setOnClickListener(v -> mPurchaseController.queryPurchaseDetails());
    root.findViewById(R.id.consume_apps).setOnClickListener(v -> {

    });
    return root;
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    LOGGER.d(TAG, "onViewCreated savedInstanceState = " + savedInstanceState);
    if (savedInstanceState != null)
    {
      BookmarkPaymentState savedState
          = BookmarkPaymentState.values()[savedInstanceState.getInt(EXTRA_CURRENT_STATE)];
      activateState(savedState);
      return;
    }
  }

  private void startPurchaseTransaction()
  {
    Framework.nativeStartPurchaseTransaction(mPaymentData.getServerId(),
                                             PrivateVariables.bookmarksVendor());
  }

  void launchBillingFlow()
  {
    mPurchaseController.launchPurchaseFlow(mPaymentData.getProductId());
  }

  @Override
  public void onStart()
  {
    super.onStart();
    Framework.nativeStartPurchaseTransactionListener(mPurchaseCallback);
    mPurchaseController.addCallback(mPurchaseCallback);
    mPurchaseCallback.attach(this);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    Framework.nativeStartPurchaseTransactionListener(null);
    mPurchaseController.removeCallback();
    mPurchaseCallback.detach();
  }

  @Override
  public void onSaveInstanceState(Bundle outState)
  {
    super.onSaveInstanceState(outState);
    LOGGER.d(TAG, "onSaveInstanceState");
    outState.putInt(EXTRA_CURRENT_STATE, mState.ordinal());
  }

  @Override
  public void onDestroy()
  {
    super.onDestroy();
    mPurchaseController.destroy();
  }

  @Override
  public void activateState(@NonNull BookmarkPaymentState state)
  {
    if (state == mState)
      return;

    LOGGER.i(TAG, "Activate state: " + state);
    mState = state;
    mState.activate(this);
  }

  private static class BookmarkPurchaseCallbackImpl
      extends StatefulPurchaseCallback<BookmarkPaymentState, BookmarkPaymentFragment>
      implements BookmarkPurchaseCallback, Detachable<BookmarkPaymentFragment>
  {
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
  }
}
