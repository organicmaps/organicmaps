package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.content.Intent;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class ChooseBookmarkCategoryActivity extends BaseMwmFragmentActivity
{
  public static final String BOOKMARK = "Bookmark";
  public static final String BOOKMARK_CATEGORY_INDEX = "BoookmarkCategoryIndex";
  public static final int REQUEST_CODE_BOOKMARK_SET = 0x1;

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return ChooseBookmarkCategoryFragment.class;
  }

  @Override
  public void onBackPressed()
  {
    setResult(Activity.RESULT_OK, new Intent().putExtra(BOOKMARK, getIntent().getParcelableExtra(BOOKMARK)));

    super.onBackPressed();
  }
}
