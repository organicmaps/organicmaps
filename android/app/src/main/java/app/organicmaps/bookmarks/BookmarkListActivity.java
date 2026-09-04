package app.organicmaps.bookmarks;

import android.content.Intent;
import android.os.Bundle;
import androidx.activity.result.ActivityResultLauncher;
import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;

public class BookmarkListActivity extends BaseBookmarksActivity
{
  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return BookmarksListFragment.class;
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
