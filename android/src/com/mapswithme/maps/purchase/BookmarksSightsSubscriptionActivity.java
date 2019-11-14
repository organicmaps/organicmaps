package com.mapswithme.maps.purchase;

import android.content.Intent;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class BookmarksSightsSubscriptionActivity extends BaseMwmFragmentActivity
{
  public static void startForResult(@NonNull FragmentActivity activity)
  {
    Intent intent = new Intent(activity, BookmarksSightsSubscriptionActivity.class);
    activity.startActivityForResult(intent, PurchaseUtils.REQ_CODE_PAY_SUBSCRIPTION);
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return BookmarksSightsSubscriptionFragment.class;
  }

  @Override
  public int getThemeResourceId(@NonNull String theme)
  {
    return R.style.MwmTheme;
  }

  public static void startForResult(@NonNull Fragment fragment, int requestCode,
                                    @NonNull String from)
  {
    Intent intent = new Intent(fragment.getActivity(), BookmarksSightsSubscriptionActivity.class);
    intent.putExtra(AbstractBookmarkSubscriptionFragment.EXTRA_FROM, from)
          .setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
    fragment.startActivityForResult(intent, requestCode);
  }
}
