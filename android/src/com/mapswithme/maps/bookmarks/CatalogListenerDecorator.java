package com.mapswithme.maps.bookmarks;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.util.DialogUtils;

class CatalogListenerDecorator extends BookmarkManager.DefaultBookmarksCatalogListener
{
  @NonNull
  private Fragment mFragment;
  @Nullable
  private BookmarkManager.BookmarksCatalogListener mWrapped;

  CatalogListenerDecorator(@NonNull Fragment fragment)
  {
    this(null, fragment);
  }

  CatalogListenerDecorator(@Nullable BookmarkManager.BookmarksCatalogListener listener,
                           @NonNull Fragment fragment)
  {
    mFragment = fragment;
    mWrapped = listener;
  }

  @Override
  public void onImportStarted(@NonNull String serverId)
  {
    if (mWrapped != null)
      mWrapped.onImportStarted(serverId);
  }

  @Override
  public void onImportFinished(@NonNull String serverId, long catId, boolean successful)
  {
    if (mWrapped != null)
      mWrapped.onImportFinished(serverId, catId, successful);

    if (successful)
      onSuccess(mFragment, catId);
    else
      onError(mFragment);
  }

  private static void onSuccess(@NonNull Fragment fragment, long catId)
  {
    BookmarkCategory category = BookmarkManager.INSTANCE.getCategoryById(catId);
    FragmentManager fm = fragment.getActivity().getSupportFragmentManager();
    ShowOnMapCatalogCategoryFragment frag =
        (ShowOnMapCatalogCategoryFragment) fm.findFragmentByTag(ShowOnMapCatalogCategoryFragment.TAG);
    if (frag == null)
    {
      ShowOnMapCatalogCategoryFragment.newInstance(category)
                                      .show(fm, ShowOnMapCatalogCategoryFragment.TAG);
      fm.executePendingTransactions();
      return;
    }

    frag.setCategory(category);
  }

  private static void onError(@NonNull Fragment fragment)
  {
    DialogUtils.showAlertDialog(fragment.getActivity(),
                                R.string.title_error_downloading_bookmarks,
                                R.string.subtitle_error_downloading_guide);
  }
}
