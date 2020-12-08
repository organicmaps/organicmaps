package com.mapswithme.maps.bookmarks;

import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.FragmentManager;
import android.view.View;

import com.cocosw.bottomsheet.BottomSheet;
import com.mapswithme.maps.R;
import com.mapswithme.maps.auth.Authorizer;
import com.mapswithme.maps.auth.TargetFragmentCallback;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.BookmarkSharingResult;
import com.mapswithme.maps.widget.BookmarkBackupView;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.sharing.SharingHelper;

public class BookmarkCategoriesFragment extends BaseBookmarkCategoriesFragment
    implements TargetFragmentCallback, AuthCompleteListener, BookmarkManager.BookmarksSharingListener
{
  @Nullable
  private BookmarkBackupController mBackupController;

  @Override
  protected void onPrepareControllers(@NonNull View view)
  {
    Authorizer authorizer = new Authorizer(this);
    BookmarkBackupView backupView = view.findViewById(R.id.backup);

    mBackupController = new BookmarkBackupController(requireActivity(), backupView, authorizer,
                                                     this);
  }

  @Override
  public void onPreparedFileForSharing(@NonNull BookmarkSharingResult result)
  {
    SharingHelper.INSTANCE.onPreparedFileForSharing(requireActivity(), result);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    BookmarkManager.INSTANCE.addSharingListener(this);
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
    BookmarkManager.INSTANCE.removeSharingListener(this);
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

  @NonNull
  @Override
  protected BookmarkCategory.Type getType()
  {
    return BookmarkCategory.Type.PRIVATE;
  }

  @Override
  public void onAuthCompleted()
  {
    FragmentManager fm = requireActivity().getSupportFragmentManager();
    BookmarkCategoriesPagerFragment pagerFragment =
        (BookmarkCategoriesPagerFragment) fm.findFragmentByTag(BookmarkCategoriesPagerFragment.class.getName());
    pagerFragment.onAuthCompleted();
  }
}
