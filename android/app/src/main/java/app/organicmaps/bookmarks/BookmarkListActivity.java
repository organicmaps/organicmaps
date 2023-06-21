package app.organicmaps.bookmarks;

import static app.organicmaps.bookmarks.BookmarksListFragment.EXTRA_BUNDLE;

import android.content.Intent;
import android.os.Bundle;

import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.StyleRes;
import androidx.fragment.app.Fragment;

import app.organicmaps.R;
import app.organicmaps.base.BaseToolbarActivity;
import app.organicmaps.bookmarks.data.BookmarkCategory;
import app.organicmaps.bookmarks.data.BookmarkManager;
import app.organicmaps.util.ThemeUtils;

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

  @SuppressWarnings("deprecation") // TODO: Remove when minSdkVersion >= 28
  static void startForResult(@NonNull Fragment fragment, @NonNull BookmarkCategory category)
  {
    Bundle args = new Bundle();
    args.putParcelable(BookmarksListFragment.EXTRA_CATEGORY, category);
    Intent intent = new Intent(fragment.requireActivity(), BookmarkListActivity.class);
    intent.putExtra(EXTRA_BUNDLE, args);
    fragment.startActivityForResult(intent, BookmarkCategoriesFragment.REQ_CODE_DELETE_CATEGORY); // Deprecated in SDK v28
  }
}
