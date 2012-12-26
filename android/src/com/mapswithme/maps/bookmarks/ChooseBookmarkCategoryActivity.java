package com.mapswithme.maps.bookmarks;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;

import android.os.Bundle;
import android.content.Intent;
import android.view.ContextMenu;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ContextMenu.ContextMenuInfo;
import android.widget.ListView;

public class ChooseBookmarkCategoryActivity extends AbstractBookmarkCategoryActivity
{
  private static final int REQUEST_CREATE_CATEGORY = 1000;
  private ChooseBookmarkCategoryAdapter mAdapter;
  private ListView mList;

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_bmk_categories);
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
    setResult(RESULT_OK, new Intent().putExtra(BookmarkActivity.PIN_SET, mAdapter.getChechedItemPosition()).
              putExtra(BookmarkActivity.PIN, getIntent().getParcelableExtra(BookmarkActivity.PIN)));
    super.onBackPressed();
  }

  @Override
  public void onCreateContextMenu(ContextMenu menu, View v,
                                  ContextMenuInfo menuInfo)
  {
    getMenuInflater().inflate(R.menu.choose_pin_sets_context_menu, menu);
    super.onCreateContextMenu(menu, v, menuInfo);
  }

  @Override
  public boolean onCreateOptionsMenu(Menu menu)
  {
    getMenuInflater().inflate(R.menu.activity_pin_sets, menu);
    return true;
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    if (item.getItemId() == R.id.pinsets_add)
    {
      startActivityForResult(
                    new Intent(this, BookmarkListActivity.class).
                    putExtra(BookmarkActivity.PIN_SET, mManager.getCategoriesCount()).
                    putExtra(BookmarkActivity.PIN, getIntent().getParcelableExtra(BookmarkActivity.PIN)).
                    putExtra(BookmarkListActivity.EDIT_CONTENT, false),
                    REQUEST_CREATE_CATEGORY);
    }
    return super.onOptionsItemSelected(item);
  }

  @Override
  protected void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    if (requestCode == REQUEST_CREATE_CATEGORY && resultCode == RESULT_OK)
    {
      mAdapter.chooseItem(data.getIntExtra(BookmarkActivity.PIN_SET, 0));
      getIntent().putExtra(BookmarkActivity.PIN, data.getParcelableExtra(BookmarkActivity.PIN));
    }
    super.onActivityResult(requestCode, resultCode, data);
  }
}
