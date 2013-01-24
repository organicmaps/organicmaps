package com.mapswithme.maps.bookmarks;

import android.content.Intent;
import android.graphics.Point;
import android.os.Bundle;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.ParcelablePoint;

public class ChooseBookmarkCategoryActivity extends AbstractBookmarkCategoryActivity
{
  private static final int REQUEST_CREATE_CATEGORY = 1000;
  private ChooseBookmarkCategoryAdapter mAdapter;

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_bmk_categories);
    setListAdapter(mAdapter = new ChooseBookmarkCategoryAdapter(this, getIntent().getIntExtra(BookmarkActivity.PIN_SET, -1)));
    getListView().setOnItemClickListener(new OnItemClickListener()
    {
      @Override
      public void onItemClick(AdapterView<?> parent, View view, int position, long id)
      {
        mAdapter.chooseItem(position);
        Point cab = ((ParcelablePoint)getIntent().getParcelableExtra(BookmarkActivity.PIN)).getPoint();
        BookmarkCategory cat = mManager.getCategoryById(position);
        mManager.getBookmark(cab.x, cab.y).setCategory(cat.getName(), position);
        getIntent().putExtra(BookmarkActivity.PIN, new ParcelablePoint(position, cat.getSize()-1));
      }
    });
    registerForContextMenu(getListView());
  }

  @Override
  public void onBackPressed()
  {
    setResult(RESULT_OK, new Intent().putExtra(BookmarkActivity.PIN, getIntent().getParcelableExtra(BookmarkActivity.PIN)));
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
                    putExtra(BookmarkListActivity.EDIT_CONTENT, enableEditing()),
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

  @Override
  protected boolean enableEditing()
  {
    return false;
  }
}
