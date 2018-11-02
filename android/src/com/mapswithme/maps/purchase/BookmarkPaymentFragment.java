package com.mapswithme.maps.purchase;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.bookmarks.data.PaymentData;

public class BookmarkPaymentFragment extends BaseMwmFragment
{
  static final String ARG_PAYMENT_DATA = "arg_payment_data";
  @SuppressWarnings("NullableProblems")
  @NonNull
  private PurchaseController<BookmarkPurchaseCallback> mPurchaseController;
  @NonNull
  private BookmarkPurchaseCallback mPurchaseCallback = new BookmarkPurchaseCallbackImpl();
  @SuppressWarnings("NullableProblems")
  @NonNull
  private PaymentData mPaymentData;
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
    mPurchaseController = PurchaseFactory.createBookmarkPurchaseController(mPaymentData.getProductId());
    mPurchaseController.initialize(getActivity());
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable
      Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_bookmark_payment, container, false);
    // TODO: temporary launch of billing flow.
    root.findViewById(R.id.buy_inapp).setOnClickListener(v -> mPurchaseController.launchPurchaseFlow(mPaymentData.getProductId()));
    root.findViewById(R.id.query_inapps).setOnClickListener(v -> mPurchaseController.queryPurchaseDetails());
    root.findViewById(R.id.consume_apps).setOnClickListener(v -> {

    });
    return root;
  }

  @Override
  public void onStart()
  {
    super.onStart();
    mPurchaseController.addCallback(mPurchaseCallback);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    mPurchaseController.removeCallback();
  }

  @Override
  public void onDestroy()
  {
    super.onDestroy();
    mPurchaseController.destroy();
  }

  private static class BookmarkPurchaseCallbackImpl implements BookmarkPurchaseCallback
  {
    // TODO: coming soon.
  }
}
