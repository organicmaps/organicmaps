package com.mapswithme.maps.purchase;

import android.content.Context;
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

  public static void start(@NonNull Context context)
  {
    Intent intent = new Intent(context, BookmarkSubscriptionActivity.class);
    context.startActivity(intent);
  }
}
