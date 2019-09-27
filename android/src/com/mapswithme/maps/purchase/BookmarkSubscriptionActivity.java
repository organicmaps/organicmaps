package com.mapswithme.maps.purchase;

import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.util.statistics.Statistics;

public class BookmarkSubscriptionActivity extends BaseMwmFragmentActivity
{
  public static void startForResult(@NonNull FragmentActivity activity)
  {
    Intent intent = new Intent(activity, BookmarkSubscriptionActivity.class)
        .putExtra(BookmarkSubscriptionFragment.EXTRA_FROM,
                  Statistics.ParamValue.SPONSORED_BUTTON);
    activity.startActivityForResult(intent, PurchaseUtils.REQ_CODE_PAY_SUBSCRIPTION);
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return BookmarkSubscriptionFragment.class;
  }

  @Override
  protected boolean useTransparentStatusBar()
  {
    return false;
  }

  public static void startForResult(@NonNull Fragment fragment, int requestCode,
                                    @NonNull String from)
  {
    Intent intent = new Intent(fragment.getActivity(), BookmarkSubscriptionActivity.class);
    intent.putExtra(BookmarkSubscriptionFragment.EXTRA_FROM, from)
          .setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
    fragment.startActivityForResult(intent, requestCode);
  }
}
