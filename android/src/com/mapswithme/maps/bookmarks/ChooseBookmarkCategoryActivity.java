package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentTransaction;

import com.mapswithme.maps.base.MWMFragmentActivity;

public class ChooseBookmarkCategoryActivity extends MWMFragmentActivity
{
  @Override
  protected void onCreate(Bundle arg0)
  {
    super.onCreate(arg0);

    getSupportActionBar().hide();

    FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
    Fragment fragment = Fragment.instantiate(this, ChooseBookmarkCategoryFragment.class.getName(), getIntent().getExtras());
    transaction.replace(android.R.id.content, fragment, "fragment");
    transaction.commit();
  }

  @Override
  public void onBackPressed()
  {
    setResult(Activity.RESULT_OK, new Intent().putExtra(BookmarkActivity.PIN,
        getIntent().getParcelableExtra(BookmarkActivity.PIN)));

    super.onBackPressed();
  }
}
