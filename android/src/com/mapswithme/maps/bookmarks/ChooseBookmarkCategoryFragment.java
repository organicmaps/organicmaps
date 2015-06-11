package com.mapswithme.maps.bookmarks;

import android.graphics.Point;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.View;
import android.widget.ListView;

import com.mapswithme.maps.base.BaseMwmListFragment;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.ParcelablePoint;
import com.mapswithme.maps.dialog.EditTextDialogFragment;
import com.mapswithme.util.statistics.Statistics;

public class ChooseBookmarkCategoryFragment extends BaseMwmListFragment implements EditTextDialogFragment.OnTextSaveListener
{
  private ChooseBookmarkCategoryAdapter mAdapter;
  private Bookmark mBookmark;

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    mAdapter = new ChooseBookmarkCategoryAdapter(getActivity(), getArguments().getInt(ChooseBookmarkCategoryActivity.BOOKMARK_SET, 0));
    setListAdapter(mAdapter);
    mBookmark = getBookmarkFromIntent();
  }

  @Override
  public void onListItemClick(ListView l, View v, int position, long id)
  {
    if (mAdapter.getItemViewType(position) == ChooseBookmarkCategoryAdapter.VIEW_TYPE_ADD_NEW)
      showCreateCategoryDialog();
    else
    {
      mAdapter.chooseItem(position);

      mBookmark.setCategoryId(position);
      getActivity().getIntent().putExtra(ChooseBookmarkCategoryActivity.BOOKMARK,
          new ParcelablePoint(mBookmark.getCategoryId(), mBookmark.getBookmarkId()));

      getActivity().onBackPressed();
    }
  }

  private Bookmark getBookmarkFromIntent()
  {
    // Note that Point result from the intent is actually a pair
    // of (category index, bookmark index in category).
    final Point cab = ((ParcelablePoint) getArguments().getParcelable(ChooseBookmarkCategoryActivity.BOOKMARK)).getPoint();
    return BookmarkManager.INSTANCE.getBookmark(cab.x, cab.y);
  }

  private void showCreateCategoryDialog()
  {
    final Bundle args = new Bundle();
    args.putString(EditTextDialogFragment.EXTRA_TITLE, "New group");
    final EditTextDialogFragment fragment = (EditTextDialogFragment) Fragment.instantiate(getActivity(), EditTextDialogFragment.class.getName());
    fragment.setOnTextSaveListener(this);
    fragment.setArguments(args);
    fragment.show(getActivity().getSupportFragmentManager(), EditTextDialogFragment.class.getName());
  }

  private void createCategory(String name)
  {
    final int category = BookmarkManager.INSTANCE.createCategory(name);
    mBookmark.setCategoryId(category);

    getActivity().getIntent().putExtra(ChooseBookmarkCategoryActivity.BOOKMARK_SET, category)
        .putExtra(ChooseBookmarkCategoryActivity.BOOKMARK, new ParcelablePoint(category, 0));

    mAdapter.chooseItem(category);

    Statistics.INSTANCE.trackGroupCreated();
  }

  @Override
  public void onSaveText(String text)
  {
    createCategory(text);
  }
}
