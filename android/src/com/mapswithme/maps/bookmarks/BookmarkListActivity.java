package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.content.Intent;

import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.StyleRes;
import androidx.fragment.app.Fragment;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseToolbarActivity;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.util.ThemeUtils;

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
    return ThemeUtils.getCardBgThemeResourceId(theme);
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
    Intent intent = new Intent(activity, BookmarkListActivity.class);
    intent.putExtra(BookmarksListFragment.EXTRA_CATEGORY, category);
    activity.startActivityForResult(intent, BaseBookmarkCategoriesFragment.REQ_CODE_DELETE_CATEGORY);
  }
}
