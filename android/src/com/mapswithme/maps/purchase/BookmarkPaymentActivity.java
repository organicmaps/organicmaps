package com.mapswithme.maps.purchase;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.bookmarks.data.PaymentData;

public class BookmarkPaymentActivity extends BaseMwmFragmentActivity
{
  public static void start(@NonNull Activity activity, @NonNull PaymentData paymentData,
                           int requestCode)
  {
    Intent intent = new Intent(activity, BookmarkPaymentActivity.class);
    Bundle args = new Bundle();
    args.putParcelable(BookmarkPaymentFragment.ARG_PAYMENT_DATA, paymentData);
    intent.putExtras(args);
    activity.startActivityForResult(intent, requestCode);
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return BookmarkPaymentFragment.class;
  }
}
