package com.mapswithme.maps.purchase;

import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class BookmarkSubscriptionActivity extends BaseMwmFragmentActivity
{
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

  public static void startForResult(@NonNull Fragment fragment, int requestCode)
  {
    Intent intent = new Intent(fragment.getActivity(), BookmarkSubscriptionActivity.class);
    intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
    fragment.startActivityForResult(intent, requestCode);
  }
}
