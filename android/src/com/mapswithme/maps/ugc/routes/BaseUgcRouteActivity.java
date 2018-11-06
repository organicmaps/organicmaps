package com.mapswithme.maps.ugc.routes;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;

public abstract class BaseUgcRouteActivity extends BaseMwmFragmentActivity
{
  public static final String EXTRA_BOOKMARK_CATEGORY = "bookmark_category";

  @Override
  protected void safeOnCreate(@Nullable Bundle savedInstanceState)
  {
    super.safeOnCreate(savedInstanceState);
    checkForResultCall();
  }

  private void checkForResultCall()
  {
    if (getCallingActivity() == null)
      throw new IllegalStateException(getClass().getSimpleName() + " must be started for result");
  }

  protected static <T> void startForResult(@NonNull Activity activity,
                                           @NonNull BookmarkCategory category,
                                           @NonNull Class<T> targetClass, int requestCode)
  {
    Intent intent = new Intent(activity, targetClass)
        .putExtra(EXTRA_BOOKMARK_CATEGORY, category);
    activity.startActivityForResult(intent, requestCode);
  }
}
