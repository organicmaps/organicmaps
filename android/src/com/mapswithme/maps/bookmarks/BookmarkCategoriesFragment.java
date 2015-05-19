package com.mapswithme.maps.bookmarks;

import android.content.Intent;
import android.os.Bundle;
import android.view.ContextMenu;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ListView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmListFragment;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;

public class BookmarkCategoriesFragment extends BaseMwmListFragment
{
  private int mSelectedPosition;
  private BookmarkCategoriesAdapter mAdapter;

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_bookmark_categories, container, false);
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    initAdapter();
    setListAdapter(mAdapter);
    registerForContextMenu(getListView());
  }

  private void initAdapter()
  {
    mAdapter = new BookmarkCategoriesAdapter(getActivity());
  }

  @Override
  public void onResume()
  {
    super.onResume();
    mAdapter.notifyDataSetChanged();
  }

  @Override
  public void onListItemClick(ListView l, View v, int position, long id)
  {
    startActivity(new Intent(getActivity(), BookmarkListActivity.class)
        .putExtra(ChooseBookmarkCategoryActivity.BOOKMARK_SET, position));
  }

  @Override
  public void onCreateContextMenu(ContextMenu menu, View v, ContextMenu.ContextMenuInfo menuInfo)
  {
    mSelectedPosition = ((AdapterView.AdapterContextMenuInfo) menuInfo).position;

    getActivity().getMenuInflater().inflate(R.menu.bookmark_categories_context_menu, menu);
    menu.setHeaderTitle(BookmarkManager.INSTANCE.getCategoryById(mSelectedPosition).getName());
  }

  @Override
  public boolean onContextItemSelected(MenuItem item)
  {
    if (item.getItemId() == R.id.set_delete)
    {
      BookmarkManager.INSTANCE.deleteCategory(mSelectedPosition);
      ((BookmarkCategoriesAdapter) getListAdapter()).notifyDataSetChanged();
    }

    return super.onContextItemSelected(item);
  }
}
