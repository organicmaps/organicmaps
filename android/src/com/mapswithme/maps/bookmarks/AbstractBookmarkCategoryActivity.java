package com.mapswithme.maps.bookmarks;

import android.content.Intent;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;

import com.mapswithme.maps.R;

public abstract class AbstractBookmarkCategoryActivity extends AbstractBookmarkListActivity
{
  private int mSelectedPosition;

  @Override
  public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo)
  {
    if (menuInfo instanceof AdapterView.AdapterContextMenuInfo)
    {
      final AdapterView.AdapterContextMenuInfo info = (AdapterView.AdapterContextMenuInfo) menuInfo;
      final BookmarkCategoriesAdapter adapter = (BookmarkCategoriesAdapter)getAdapter();
      if (adapter.isActiveItem(info.position))
      {
        mSelectedPosition = info.position;
        menu.setHeaderTitle(mManager.getCategoryById(mSelectedPosition).getName());
      }
    }
    super.onCreateContextMenu(menu, v, menuInfo);
  }

  @Override
  public boolean onContextItemSelected(MenuItem item)
  {
    final int itemId = item.getItemId();
    if (itemId == R.id.set_edit)
    {
      startActivity(new Intent(this, BookmarkListActivity.class).
                    putExtra(BookmarkActivity.PIN_SET, mSelectedPosition).
                    putExtra(BookmarkListActivity.EDIT_CONTENT, enableEditing()));
    }
    else if (itemId == R.id.set_delete)
    {
      mManager.deleteCategory(mSelectedPosition);
      getAdapter().notifyDataSetChanged();
    }
    return super.onContextItemSelected(item);
  }

  protected abstract boolean enableEditing();

  protected AbstractBookmarkCategoryAdapter getAdapter()
  {
    return ((AbstractBookmarkCategoryAdapter) getListView().getAdapter());
  }

  @Override
  protected void onStart()
  {
    super.onStart();
    getAdapter().notifyDataSetChanged();
  }
}
