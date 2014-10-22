package com.mapswithme.maps.bookmarks;

import android.content.Intent;
import android.os.Bundle;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ListView;

import com.mapswithme.maps.R;

public class BookmarkCategoriesActivity extends AbstractBookmarkCategoryActivity
{
  private int mSelectedPosition;

  @Override
  protected BookmarkCategoriesAdapter getAdapter()
  {
    return (BookmarkCategoriesAdapter) getListView().getAdapter();
  }

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.categories);

    final ListView lv = getListView();
    lv.setAdapter(new BookmarkCategoriesAdapter(this));
    lv.setOnItemClickListener(new OnItemClickListener()
    {
      @Override
      public void onItemClick(AdapterView<?> parent, View view, int position, long id)
      {
        startActivity(new Intent(BookmarkCategoriesActivity.this, BookmarkListActivity.class)
            .putExtra(BookmarkActivity.PIN_SET, position));
      }
    });

    registerForContextMenu(getListView());
  }

  @Override
  public void onCreateContextMenu(ContextMenu menu, View v,
                                  ContextMenuInfo menuInfo)
  {
    mSelectedPosition = ((AdapterView.AdapterContextMenuInfo) menuInfo).position;
    getMenuInflater().inflate(R.menu.bookmark_categories_context_menu, menu);
    menu.setHeaderTitle(mManager.getCategoryById(mSelectedPosition).getName());

    super.onCreateContextMenu(menu, v, menuInfo);
  }

  @Override
  public boolean onContextItemSelected(MenuItem item)
  {
    if (item.getItemId() == R.id.set_delete)
    {
      assert (mSelectedPosition != -1);
      mManager.deleteCategory(mSelectedPosition);
      getAdapter().notifyDataSetChanged();
    }

    return super.onContextItemSelected(item);
  }
}
