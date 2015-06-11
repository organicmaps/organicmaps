package com.mapswithme.maps.bookmarks;

import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.ContextMenu;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ListView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmListFragment;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.dialog.EditTextDialogFragment;

public class BookmarkCategoriesFragment extends BaseMwmListFragment implements EditTextDialogFragment.OnTextSaveListener
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

    getActivity().getMenuInflater().inflate(R.menu.menu_bookmark_categories, menu);
    menu.setHeaderTitle(BookmarkManager.INSTANCE.getCategoryById(mSelectedPosition).getName());
  }

  @Override
  public boolean onContextItemSelected(MenuItem item)
  {
    switch (item.getItemId())
    {
    case R.id.set_delete:
      BookmarkManager.INSTANCE.deleteCategory(mSelectedPosition);
      ((BookmarkCategoriesAdapter) getListAdapter()).notifyDataSetChanged();
      break;
    case R.id.set_edit:
      final Bundle args = new Bundle();
      args.putString(EditTextDialogFragment.EXTRA_TITLE, getString(R.string.bookmark_set_name));
      args.putString(EditTextDialogFragment.EXTRA_POSITIVE_BUTTON, getString(R.string.edit));
      final EditTextDialogFragment fragment = (EditTextDialogFragment) Fragment.instantiate(getActivity(), EditTextDialogFragment.class.getName());
      fragment.setOnTextSaveListener(this);
      fragment.setArguments(args);
      fragment.show(getActivity().getSupportFragmentManager(), EditTextDialogFragment.class.getName());
      break;
    }

    return super.onContextItemSelected(item);
  }

  @Override
  public void onSaveText(String text)
  {
    final BookmarkCategory category = BookmarkManager.INSTANCE.getCategoryById(mSelectedPosition);
    category.setName(text);
    mAdapter.notifyDataSetChanged();
  }
}
