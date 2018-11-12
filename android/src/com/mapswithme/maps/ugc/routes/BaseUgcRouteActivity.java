package com.mapswithme.maps.ugc.routes;

import android.app.Activity;
import android.content.Intent;
import android.support.annotation.NonNull;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;

public abstract class BaseUgcRouteActivity extends BaseMwmFragmentActivity
{
  public static final String EXTRA_BOOKMARK_CATEGORY = "bookmark_category";

  protected static <T> void startForResult(@NonNull Activity activity,
                                           @NonNull BookmarkCategory category,
                                           @NonNull Class<T> targetClass, int requestCode)
  {
    Intent intent = new Intent(activity, targetClass)
        .putExtra(EXTRA_BOOKMARK_CATEGORY, category);
    activity.startActivityForResult(intent, requestCode);
  }
}
