package app.organicmaps.bookmarks;

import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import androidx.activity.SystemBarStyle;
import androidx.activity.result.ActivityResultLauncher;
import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import app.organicmaps.R;
import app.organicmaps.base.BaseToolbarActivity;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;

public class BookmarkListActivity extends BaseToolbarActivity
{
  /**
   * The toolbar continues the ?cardBackground surface of the list instead of the branded bar, so the status bar
   * icons have to follow the theme - light on the dark surface, dark on the light one - rather than stay light.
   */
  @NonNull
  @Override
  protected SystemBarStyle getStatusBarStyle()
  {
    return SystemBarStyle.auto(Color.TRANSPARENT, Color.TRANSPARENT);
  }

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
  protected Class<? extends Fragment> getFragmentClass()
  {
    return BookmarksListFragment.class;
  }

  @Override
  protected int getContentLayoutResId()
  {
    return R.layout.bookmark_list_activity;
  }

  static void startForResult(@NonNull Fragment fragment, ActivityResultLauncher<Intent> startBookmarkListForResult,
                             @NonNull BookmarkCategory category)
  {
    Bundle args = new Bundle();
    Intent intent = new Intent(fragment.requireActivity(), BookmarkListActivity.class);
    intent.putExtra(BookmarksListFragment.EXTRA_CATEGORY, category);
    startBookmarkListForResult.launch(intent);
  }
}
