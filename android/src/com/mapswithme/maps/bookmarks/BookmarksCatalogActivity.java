package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseToolbarActivity;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;

public class BookmarksCatalogActivity extends BaseToolbarActivity
{
  public static final int REQ_CODE_CATALOG = 101;
  public static final String EXTRA_DOWNLOADED_CATEGORY = "extra_downloaded_category";

  public static void startForResult(@NonNull Fragment fragment, int requestCode)
  {
    fragment.startActivityForResult(makeLaunchIntent(fragment.getContext()), requestCode);
  }

  public static void startForResult(@NonNull Activity context, int requestCode)
  {
    context.startActivityForResult(makeLaunchIntent(context), requestCode);
  }

  @NonNull
  private static Intent makeLaunchIntent(@NonNull Context context)
  {
    Intent intent = new Intent(context, BookmarksCatalogActivity.class);
    intent.putExtra(BookmarksCatalogFragment.EXTRA_BOOKMARKS_CATALOG_URL,
                    BookmarkManager.INSTANCE.getCatalogFrontendUrl());
    intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
    return intent;
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return BookmarksCatalogFragment.class;
  }

  @Override
  protected void onHomeOptionItemSelected()
  {
    finish();
  }
}
