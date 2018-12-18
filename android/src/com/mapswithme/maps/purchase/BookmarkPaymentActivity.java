package com.mapswithme.maps.purchase;

import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.bookmarks.data.PaymentData;

public class BookmarkPaymentActivity extends BaseMwmFragmentActivity
{
  public static void startForResult(@NonNull Fragment fragment, @NonNull PaymentData paymentData,
                                    int requestCode)
  {
    Intent intent = new Intent(fragment.getActivity(), BookmarkPaymentActivity.class);
    intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
    Bundle args = new Bundle();
    args.putParcelable(BookmarkPaymentFragment.ARG_PAYMENT_DATA, paymentData);
    intent.putExtras(args);
    fragment.startActivityForResult(intent, requestCode);
  }

  @Override
  protected void safeOnCreate(@Nullable Bundle savedInstanceState)
  {
    super.safeOnCreate(savedInstanceState);
    overridePendingTransition(0, 0);
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return BookmarkPaymentFragment.class;
  }
}
