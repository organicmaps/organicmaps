package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentTransaction;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class ChooseBookmarkCategoryActivity extends BaseMwmFragmentActivity
{
  public static final String BOOKMARK = "pin";
  public static final String BOOKMARK_SET = "pin_set";
  public static final int REQUEST_CODE_SET = 0x1;
  public static final int REQUEST_CODE_EDIT_BOOKMARK = 0x2;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
    Fragment fragment = Fragment.instantiate(this, ChooseBookmarkCategoryFragment.class.getName(), getIntent().getExtras());
    transaction.replace(android.R.id.content, fragment, "fragment");
    transaction.commit();
  }

  @Override
  public void onBackPressed()
  {
    setResult(Activity.RESULT_OK, new Intent().putExtra(BOOKMARK, getIntent().getParcelableExtra(BOOKMARK)));

    super.onBackPressed();
  }
}
