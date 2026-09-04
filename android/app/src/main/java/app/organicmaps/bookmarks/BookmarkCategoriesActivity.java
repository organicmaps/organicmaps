package app.organicmaps.bookmarks;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;

public class BookmarkCategoriesActivity extends BaseBookmarksActivity
{
  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return BookmarkCategoriesFragment.class;
  }

  public static void start(@NonNull Activity context, @Nullable BookmarkCategory category)
  {
    Bundle args = new Bundle();
    args.putParcelable(BookmarksListFragment.EXTRA_CATEGORY, category);
    Intent intent = new Intent(context, BookmarkCategoriesActivity.class);
    intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP).putExtras(args);
    context.startActivity(intent);
  }

  public static void start(@NonNull Activity context)
  {
    start(context, null);
  }
}
