package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.StyleRes;
import androidx.fragment.app.Fragment;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseToolbarActivity;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.util.ThemeUtils;

import static com.mapswithme.maps.bookmarks.BookmarksListFragment.EXTRA_BUNDLE;

public class BookmarkListActivity extends BaseToolbarActivity
{
  @CallSuper
  @Override
  public void onResume()
  {
    super.onResume();

    // Disable all notifications in BM on appearance of this activity.
    // It allows to significantly improve performance in case of bookmarks
    // modification. All notifications will be sent on activity's disappearance.
    BookmarkManager.INSTANCE.setNotificationsEnabled(false);
  }

  @CallSuper
  @Override
  public void onPause()
  {
    // Allow to send all notifications in BM.
    BookmarkManager.INSTANCE.setNotificationsEnabled(true);

    super.onPause();
  }

  @Override
  @StyleRes
  public int getThemeResourceId(@NonNull String theme)
  {
    return ThemeUtils.getCardBgThemeResourceId(getApplicationContext(), theme);
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return BookmarksListFragment.class;
  }

  @Override
  protected int getContentLayoutResId()
  {
    return R.layout.bookmarks_activity;
  }

  static void startForResult(@NonNull Activity activity, @NonNull BookmarkCategory category)
  {
    activity.startActivityForResult(getStartIntent(activity, category),
                                    BaseBookmarkCategoriesFragment.REQ_CODE_DELETE_CATEGORY);
  }

  @NonNull
  static Intent getStartIntent(@NonNull Context context,
                                        @NonNull BookmarkCategory bookmarkCategory)
  {
    Intent intent = new Intent(context, BookmarkListActivity.class);
    return wrapDataToBundle(intent, bookmarkCategory);
  }

  @NonNull
  private static Intent wrapDataToBundle(@NonNull Intent intent,
                                                  @NonNull BookmarkCategory bookmarkCategory)
  {
    Bundle bundle = new Bundle();
    bundle.putParcelable(BookmarksListFragment.EXTRA_CATEGORY, bookmarkCategory);
    intent.putExtra(EXTRA_BUNDLE, bundle);
    return intent;
  }
}
