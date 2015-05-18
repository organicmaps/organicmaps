package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentTransaction;

import com.mapswithme.maps.base.BaseMwmFragmentActivity;

public class ChooseBookmarkCategoryActivity extends BaseMwmFragmentActivity
{
  public static final String PIN = "pin";
  public static final String PIN_ICON_ID = "pin";
  public static final String PIN_SET = "pin_set";
  public static final int REQUEST_CODE_SET = 0x1;
  public static final String BOOKMARK_NAME = "bookmark_name";
  public static final int REQUEST_CODE_EDIT_BOOKMARK = 0x2;

  @Override
  protected void onCreate(Bundle arg0)
  {
    super.onCreate(arg0);

    FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
    Fragment fragment = Fragment.instantiate(this, ChooseBookmarkCategoryFragment.class.getName(), getIntent().getExtras());
    transaction.replace(android.R.id.content, fragment, "fragment");
    transaction.commit();
  }

  @Override
  public void onBackPressed()
  {
    setResult(Activity.RESULT_OK, new Intent().putExtra(PIN, getIntent().getParcelableExtra(PIN)));

    super.onBackPressed();
  }
}
