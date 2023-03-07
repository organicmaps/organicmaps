package app.organicmaps.bookmarks;

import android.app.Activity;
import android.app.ProgressDialog;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.FragmentActivity;

import app.organicmaps.R;
import app.organicmaps.bookmarks.data.BookmarkManager;
import app.organicmaps.bookmarks.data.BookmarkSharingResult;
import app.organicmaps.util.SharingUtils;
import app.organicmaps.util.log.Logger;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;

public enum BookmarksSharingHelper
{
  INSTANCE;

  private static final String TAG = BookmarksSharingHelper.class.getSimpleName();

  @Nullable
  private ProgressDialog mProgressDialog;

  public void prepareBookmarkCategoryForSharing(@NonNull Activity context, long catId)
  {
    mProgressDialog = new ProgressDialog(context, R.style.MwmTheme_AlertDialog);
    mProgressDialog.setMessage(context.getString(R.string.please_wait));
    mProgressDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
    mProgressDialog.setIndeterminate(true);
    mProgressDialog.setCancelable(false);
    mProgressDialog.show();
    BookmarkManager.INSTANCE.prepareCategoryForSharing(catId);
  }

  public void onPreparedFileForSharing(@NonNull FragmentActivity context,
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
        new MaterialAlertDialogBuilder(context, R.style.MwmTheme_AlertDialog)
            .setTitle(R.string.bookmarks_error_title_share_empty)
            .setMessage(R.string.bookmarks_error_message_share_empty)
            .setPositiveButton(R.string.ok, null)
            .show();
        break;
      case BookmarkSharingResult.ARCHIVE_ERROR:
      case BookmarkSharingResult.FILE_ERROR:
        new MaterialAlertDialogBuilder(context, R.style.MwmTheme_AlertDialog)
            .setTitle(R.string.dialog_routing_system_error)
            .setMessage(R.string.bookmarks_error_message_share_general)
            .setPositiveButton(R.string.ok, null)
            .show();
        String catName = BookmarkManager.INSTANCE.getCategoryById(result.getCategoryId()).getName();
        Logger.e(TAG, "Failed to share bookmark category '" + catName + "', error code: " + result.getCode());
        break;
      default:
        throw new AssertionError("Unsupported bookmark sharing code: " + result.getCode());
    }
  }
}
