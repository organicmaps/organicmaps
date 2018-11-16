package com.mapswithme.maps.purchase;

import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.bookmarks.data.PaymentData;
import com.mapswithme.util.Utils;

public class BookmarkPaymentActivity extends BaseMwmFragmentActivity
{
  public static void startForResult(@NonNull Fragment fragment, @NonNull PaymentData paymentData,
                                    int requestCode)
  {
    Intent intent = new Intent(fragment.getActivity(), BookmarkPaymentActivity.class);
    Bundle args = new Bundle();
    args.putParcelable(BookmarkPaymentFragment.ARG_PAYMENT_DATA, paymentData);
    intent.putExtras(args);
    fragment.startActivityForResult(intent, requestCode);
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return BookmarkPaymentFragment.class;
  }

  @Override
  protected boolean onBackPressedInternal(@NonNull Fragment currentFragment)
  {
    return Utils.<OnBackPressListener>castTo(currentFragment).onBackPressed();
  }
}
