package com.mapswithme.maps.bookmarks;

import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseToolbarActivity;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.util.Utils;

public class BookmarksCatalogActivity extends BaseToolbarActivity
{
  public static final int REQ_CODE_CATALOG = 123;
  public static final String EXTRA_CATEGORY = "category";

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return BookmarksCatalogFragment.class;
  }

  @Override
  protected boolean onBackPressedInternal(@NonNull Fragment currentFragment)
  {
    return Utils.<OnBackPressListener>castTo(currentFragment).onBackPressed();
  }

  @Override
  protected void onHomeOptionItemSelected()
  {
    finish();
  }
}
