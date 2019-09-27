package com.mapswithme.maps.bookmarks;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.dialog.DialogUtils;

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
