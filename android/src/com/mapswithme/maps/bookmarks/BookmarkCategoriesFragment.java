package com.mapswithme.maps.bookmarks;

import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.CallSuper;
import android.support.annotation.LayoutRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.view.MenuItem;
import android.view.View;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmRecyclerFragment;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.dialog.EditTextDialogFragment;
import com.mapswithme.maps.widget.PlaceholderView;
import com.mapswithme.maps.widget.recycler.RecyclerClickListener;
import com.mapswithme.maps.widget.recycler.RecyclerLongClickListener;
import com.mapswithme.util.BottomSheetHelper;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.sharing.SharingHelper;

public class BookmarkCategoriesFragment extends BaseMwmRecyclerFragment
                                     implements EditTextDialogFragment.OnTextSaveListener,
                                                MenuItem.OnMenuItemClickListener,
                                                RecyclerClickListener,
                                                RecyclerLongClickListener,
                                                BookmarkManager.BookmarksLoadingListener
{
  private int mSelectedPosition;
  @Nullable
  private View mLoadingPlaceholder;

  @Override
  protected @LayoutRes int getLayoutRes()
  {
    return R.layout.fragment_bookmark_categories;
  }

  @Override
  protected RecyclerView.Adapter createAdapter()
  {
    return new BookmarkCategoriesAdapter(getActivity());
  }

  @Nullable
  @Override
  protected BookmarkCategoriesAdapter getAdapter()
  {
    RecyclerView.Adapter adapter = super.getAdapter();
    return adapter != null ? (BookmarkCategoriesAdapter) adapter : null;
  }

  @CallSuper
  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    mLoadingPlaceholder = view.findViewById(R.id.placeholder_loading);

    if (getAdapter() != null)
    {
      getAdapter().setOnClickListener(this);
      getAdapter().setOnLongClickListener(this);
      getAdapter().registerAdapterDataObserver(new RecyclerView.AdapterDataObserver()
      {
        @Override
        public void onChanged()
        {
          updateResultsPlaceholder();
        }
      });
    }
  }

  private void updateResultsPlaceholder()
  {
    if (getAdapter() != null)
    {
      boolean showLoadingPlaceholder = BookmarkManager.nativeIsAsyncBookmarksLoadingInProgress();
      showPlaceholder(!showLoadingPlaceholder && getAdapter().getItemCount() == 0);
    }
  }

  private void updateLoadingPlaceholder()
  {
    if (mLoadingPlaceholder != null)
    {
      boolean showLoadingPlaceholder = BookmarkManager.nativeIsAsyncBookmarksLoadingInProgress();
      if (getAdapter() != null && getAdapter().getItemCount() != 0)
        showLoadingPlaceholder = false;

      UiUtils.showIf(showLoadingPlaceholder, mLoadingPlaceholder);
    }
  }

  @Override
  public void onStart()
  {
    super.onStart();
    BookmarkManager.INSTANCE.addListener(this);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    BookmarkManager.INSTANCE.removeListener(this);
  }

  @Override
  public void onResume()
  {
    super.onResume();
    updateLoadingPlaceholder();
    if (getAdapter() != null)
      getAdapter().notifyDataSetChanged();
  }

  @Override
  public void onPause()
  {
    super.onPause();
    BottomSheetHelper.free();
  }

  @Override
  public void onSaveText(String text)
  {
    final BookmarkCategory category = BookmarkManager.INSTANCE.getCategory(mSelectedPosition);
    category.setName(text);
    if (getAdapter() != null)
      getAdapter().notifyDataSetChanged();
  }

  @Override
  public boolean onMenuItemClick(MenuItem item)
  {
    switch (item.getItemId())
    {
    case R.id.set_show:
      BookmarkManager.INSTANCE.toggleCategoryVisibility(mSelectedPosition);
      if (getAdapter() != null)
        getAdapter().notifyDataSetChanged();
      break;

    case R.id.set_share:
      SharingHelper.shareBookmarksCategory(getActivity(), mSelectedPosition);
      break;

    case R.id.set_delete:
      BookmarkManager.INSTANCE.nativeDeleteCategory(mSelectedPosition);
      if (getAdapter() != null)
        getAdapter().notifyDataSetChanged();
      break;

    case R.id.set_edit:
      EditTextDialogFragment.show(getString(R.string.bookmark_set_name),
                                  BookmarkManager.INSTANCE.getCategory(mSelectedPosition).getName(),
                                  getString(R.string.rename), getString(R.string.cancel), this);
      break;
    }

    return true;
  }

  @Override
  public void onLongItemClick(View v, int position)
  {
    mSelectedPosition = position;

    BookmarkCategory category = BookmarkManager.INSTANCE.getCategory(mSelectedPosition);
    BottomSheetHelper.Builder bs = BottomSheetHelper.create(getActivity(), category.getName())
                                                    .sheet(R.menu.menu_bookmark_categories)
                                                    .listener(this);
    MenuItem show = bs.getMenu().getItem(0);
    show.setIcon(category.isVisible() ? R.drawable.ic_hide
                                      : R.drawable.ic_show);
    show.setTitle(category.isVisible() ? R.string.hide
                                       : R.string.show);
    bs.tint().show();
  }

  @Override
  public void onItemClick(View v, int position)
  {
    startActivity(new Intent(getActivity(), BookmarkListActivity.class)
                      .putExtra(ChooseBookmarkCategoryFragment.CATEGORY_ID, position));
  }

  @Override
  protected void setupPlaceholder(@NonNull PlaceholderView placeholder)
  {
    placeholder.setContent(R.drawable.img_bookmarks, R.string.bookmarks_empty_title,
        R.string.bookmarks_usage_hint);
  }

  @Override
  public void onBookmarksLoadingStarted()
  {
    updateLoadingPlaceholder();
    updateResultsPlaceholder();
  }

  @Override
  public void onBookmarksLoadingFinished()
  {
    updateLoadingPlaceholder();
    updateResultsPlaceholder();
    if (getAdapter() != null)
      getAdapter().notifyDataSetChanged();
  }

  @Override
  public void onBookmarksFileLoaded(boolean success)
  {
    // Do nothing here.
  }
}
