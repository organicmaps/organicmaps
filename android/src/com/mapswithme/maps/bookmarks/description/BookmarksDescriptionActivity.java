package com.mapswithme.maps.bookmarks.description;

import android.content.Context;
import android.content.Intent;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import com.mapswithme.maps.base.BaseToolbarActivity;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;

public class BookmarksDescriptionActivity extends BaseToolbarActivity
{

  static final String EXTRA_CATEGORY = "BookmarksDescriptionActivity.bookmark_category_extra_data";

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return BookmarksDescriptionFragment.class;
  }

  public static void start(@NonNull Context context, @NonNull BookmarkCategory data)
  {
    Intent intent = new Intent(context, BookmarksDescriptionActivity.class)
        .putExtra(EXTRA_CATEGORY, data);
    context.startActivity(intent);
  }
}
