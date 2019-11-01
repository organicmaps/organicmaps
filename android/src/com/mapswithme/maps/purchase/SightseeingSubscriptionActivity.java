package com.mapswithme.maps.purchase;

import android.content.Intent;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class SightseeingSubscriptionActivity extends BaseMwmFragmentActivity
{
  public static void startForResult(@NonNull FragmentActivity activity)
  {
    Intent intent = new Intent(activity, SightseeingSubscriptionActivity.class);
    activity.startActivityForResult(intent, PurchaseUtils.REQ_CODE_PAY_SUBSCRIPTION);
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return SightseeingSubscriptionFragment.class;
  }

  @Override
  public int getThemeResourceId(@NonNull String theme)
  {
    return R.style.MwmTheme;
  }
}
