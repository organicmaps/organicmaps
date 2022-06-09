package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.app.ProgressDialog;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.BookmarkSharingResult;
import com.mapswithme.maps.dialog.DialogUtils;
import com.mapswithme.util.SharingUtils;
import com.mapswithme.util.log.Logger;

public enum BookmarksSharingHelper
{
  INSTANCE;

  private static final String TAG = BookmarksSharingHelper.class.getSimpleName();

  @Nullable
  private ProgressDialog mProgressDialog;

  public void prepareBookmarkCategoryForSharing(@NonNull Activity context, long catId)
  {
    mProgressDialog = DialogUtils.createModalProgressDialog(context, R.string.please_wait);
    mProgressDialog.show();
    BookmarkManager.INSTANCE.prepareCategoryForSharing(catId);
  }

  public void onPreparedFileForSharing(@NonNull Activity context,
                                       @NonNull BookmarkSharingResult result)
  {
    if (mProgressDialog != null && mProgressDialog.isShowing())
      mProgressDialog.dismiss();

    switch (result.getCode())
    {
      case BookmarkSharingResult.SUCCESS:
        SharingUtils.shareBookmarkFile(context, result.getSharingPath());
        break;
      case BookmarkSharingResult.EMPTY_CATEGORY:
        DialogUtils.showAlertDialog(context, R.string.bookmarks_error_title_share_empty,
                                    R.string.bookmarks_error_message_share_empty);
        break;
      case BookmarkSharingResult.ARCHIVE_ERROR:
      case BookmarkSharingResult.FILE_ERROR:
        DialogUtils.showAlertDialog(context, R.string.dialog_routing_system_error,
                                    R.string.bookmarks_error_message_share_general);
        String catName = BookmarkManager.INSTANCE.getCategoryById(result.getCategoryId()).getName();
        Logger.e(TAG, "Failed to share bookmark category '" + catName + "', error code: " + result.getCode());
        break;
      default:
        throw new AssertionError("Unsupported bookmark sharing code: " + result.getCode());
    }
  }
}
