package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;
import android.support.v7.widget.Toolbar;

import com.mapswithme.maps.base.BaseToolbarActivity;

public class BookmarksCatalogActivity extends BaseToolbarActivity
{
  public static final String EXTRA_DOWNLOADED_CATEGORY = "extra_downloaded_category";

  public static void startForResult(@NonNull Fragment fragment, int requestCode,
                                    @NonNull String catalogUrl)
  {
    fragment.startActivityForResult(makeLaunchIntent(fragment.requireContext(), catalogUrl),
                                    requestCode);
  }

  public static void startForResult(@NonNull Activity context, int requestCode,
                                    @NonNull String catalogUrl)
  {
    context.startActivityForResult(makeLaunchIntent(context, catalogUrl), requestCode);
  }

  public static void start(@NonNull Context context, @NonNull String catalogUrl)
  {
    context.startActivity(makeLaunchIntent(context, catalogUrl));
  }

  @NonNull
  private static Intent makeLaunchIntent(@NonNull Context context, @NonNull String catalogUrl)
  {
    Intent intent = new Intent(context, BookmarksCatalogActivity.class);
    intent.putExtra(BookmarksCatalogFragment.EXTRA_BOOKMARKS_CATALOG_URL, catalogUrl);
    intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
    return intent;
  }

  @Override
  protected void setupHomeButton(@NonNull Toolbar toolbar)
  {
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
