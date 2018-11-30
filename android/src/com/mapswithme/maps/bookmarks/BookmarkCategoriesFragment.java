package com.mapswithme.maps.bookmarks;

import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.View;

import com.cocosw.bottomsheet.BottomSheet;
import com.mapswithme.maps.R;
import com.mapswithme.maps.auth.Authorizer;
import com.mapswithme.maps.auth.TargetFragmentCallback;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.widget.BookmarkBackupView;
import com.mapswithme.util.UiUtils;

public class BookmarkCategoriesFragment extends BaseBookmarkCategoriesFragment
    implements TargetFragmentCallback
{
  @Nullable
  private BookmarkBackupController mBackupController;

  @Override
  protected void onPrepareControllers(@NonNull View view)
  {
    super.onPrepareControllers(view);
    Authorizer authorizer = new Authorizer(this);
    BookmarkBackupView backupView = view.findViewById(R.id.backup);
    mBackupController = new BookmarkBackupController(getActivity(), backupView, authorizer);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    if (mBackupController != null)
      mBackupController.onStart();
  }

  @Override
  protected void updateLoadingPlaceholder()
  {
    super.updateLoadingPlaceholder();
    boolean isLoading = BookmarkManager.INSTANCE.isAsyncBookmarksLoadingInProgress();
    UiUtils.showIf(!isLoading, getView(), R.id.backup, R.id.recycler);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    if (mBackupController != null)
      mBackupController.onStop();
  }

  @Override
  public void onTargetFragmentResult(int resultCode, @Nullable Intent data)
  {
    if (mBackupController != null)
      mBackupController.onTargetFragmentResult(resultCode, data);
  }

  @Override
  public boolean isTargetAdded()
  {
    return isAdded();
  }

  @Override
  protected void prepareBottomMenuItems(@NonNull BottomSheet bottomSheet)
  {
    boolean isMultipleItems = getAdapter().getBookmarkCategories().size() > 1;
    setEnableForMenuItem(R.id.delete, bottomSheet, isMultipleItems);
    setEnableForMenuItem(R.id.sharing_options, bottomSheet,
                         getSelectedCategory().isSharingOptionsAllowed());
  }
}
