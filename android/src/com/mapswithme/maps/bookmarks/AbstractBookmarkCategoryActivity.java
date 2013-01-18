package com.mapswithme.maps.bookmarks;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;

import android.content.Intent;
import android.os.Bundle;
import android.view.ContextMenu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ContextMenu.ContextMenuInfo;
import android.widget.AdapterView;

public class AbstractBookmarkCategoryActivity extends AbstractBookmarkListActivity
{
  private int mSelectedPosition;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
  }

  @Override
  public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
  {
    if (menuInfo instanceof AdapterView.AdapterContextMenuInfo)
    {
      AdapterView.AdapterContextMenuInfo info = (AdapterView.AdapterContextMenuInfo) menuInfo;
      mSelectedPosition = info.position;
    }
    super.onCreateContextMenu(menu, v, menuInfo);
  }

  @Override
  public boolean onContextItemSelected(MenuItem item)
  {
    int itemId = item.getItemId();
    if (itemId == R.id.set_edit)
    {
      startActivity(new Intent(this, BookmarkListActivity.class).putExtra(BookmarkActivity.PIN_SET, mSelectedPosition));
    }
    else if (itemId == R.id.set_delete)
    {
      mManager.deleteCategory(mSelectedPosition);
     // ((AbstractPinSetAdapter) getListView().getAdapter()).remove(set);
      ((AbstractBookmarkCategoryAdapter) getListView().getAdapter()).notifyDataSetChanged();
    }
    return super.onContextItemSelected(item);
  }

  @Override
  protected void onStart()
  {
    super.onStart();
    ((AbstractBookmarkCategoryAdapter) getListView().getAdapter()).notifyDataSetChanged();
  }
}
