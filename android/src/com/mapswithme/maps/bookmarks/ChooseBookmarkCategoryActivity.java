package com.mapswithme.maps.bookmarks;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;

import android.os.Bundle;
import android.content.Intent;
import android.view.Menu;
import android.widget.ListView;

public class ChooseBookmarkCategoryActivity extends AbstractBookmarkCategoryActivity
{
  private ChooseBookmarkCategoryAdapter mAdapter;
  private ListView mList;

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_pin_sets);
    mList = (ListView) findViewById(android.R.id.list);
    mList.setAdapter(mAdapter = new ChooseBookmarkCategoryAdapter(this, getIntent().getIntExtra(BookmarkActivity.PIN_SET, -1)));
    int checked = getIntent().getIntExtra(BookmarkActivity.PIN_SET, -1);
    mList.setItemChecked(checked, true);
    mList.setSelection(checked);
    mList.setOnItemClickListener(mAdapter);
    registerForContextMenu(getListView());
  }

  @Override
  public void onBackPressed()
  {
    setResult(RESULT_OK, new Intent().putExtra(BookmarkActivity.PIN_SET, mList.getCheckedItemPosition()));
    super.onBackPressed();
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu)
  {
    getMenuInflater().inflate(R.menu.activity_pin_sets, menu);
    return true;
  }
}
