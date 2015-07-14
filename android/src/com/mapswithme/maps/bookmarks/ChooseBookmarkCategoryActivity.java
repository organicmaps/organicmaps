package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.content.Intent;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class ChooseBookmarkCategoryActivity extends BaseMwmFragmentActivity
{
  public static final String BOOKMARK = "pin";
  public static final String BOOKMARK_SET = "pin_set";
  public static final int REQUEST_CODE_SET = 0x1;
  public static final int REQUEST_CODE_EDIT_BOOKMARK = 0x2;

  @Override
  protected String getFragmentClassName()
  {
    return ChooseBookmarkCategoryFragment.class.getName();
  }

  @Override
  public void onBackPressed()
  {
    setResult(Activity.RESULT_OK, new Intent().putExtra(BOOKMARK, getIntent().getParcelableExtra(BOOKMARK)));

    super.onBackPressed();
  }
}
