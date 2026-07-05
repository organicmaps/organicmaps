package app.organicmaps.bookmarks;

import android.app.ProgressDialog;
import androidx.activity.result.ActivityResultLauncher;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.FragmentActivity;
import app.organicmaps.R;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager.BookmarksSharingListener;
import app.organicmaps.sdk.bookmarks.data.BookmarkSharingResult;
import app.organicmaps.sdk.bookmarks.data.FileType;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.util.SharingUtils;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import java.util.ArrayList;
import java.util.List;

/// Owns the whole "prepare file → share" flow for bookmarks and tracks.
///
/// File preparation in the core is asynchronous and its single completion callback is broadcast to
/// every registered listener. To avoid handling one result on several screens at once (which would
/// pop up duplicate share sheets), this singleton is the only registered listener and remembers the
/// one requester that started the in-flight share. The result is delivered to exactly that owner and
/// then forgotten.
public enum BookmarksSharingHelper implements BookmarksSharingListener
{
  INSTANCE;

  private static final String TAG = BookmarksSharingHelper.class.getSimpleName();

  @Nullable
  private ProgressDialog mProgressDialog;
  // The single in-flight share request owner, set when a share starts and cleared on its result.
  @Nullable
  private FragmentActivity mActivity;
  @Nullable
  private ActivityResultLauncher<SharingUtils.SharingIntent> mLauncher;
  private boolean mListenerRegistered;

  public void prepareBookmarkCategoryForSharing(@NonNull FragmentActivity activity,
                                                @NonNull ActivityResultLauncher<SharingUtils.SharingIntent> launcher,
                                                long catId, @NonNull FileType fileType)
  {
    beginSharing(activity, launcher);
    BookmarkManager.INSTANCE.prepareCategoriesForSharing(new long[] {catId}, fileType);
  }

  public void prepareTrackForSharing(@NonNull FragmentActivity activity,
                                     @NonNull ActivityResultLauncher<SharingUtils.SharingIntent> launcher, long trackId,
                                     @NonNull FileType fileType)
  {
    beginSharing(activity, launcher);
    BookmarkManager.INSTANCE.prepareTrackForSharing(trackId, fileType);
  }

  public void prepareBookmarkCategoriesForSharing(@NonNull FragmentActivity activity,
                                                  @NonNull ActivityResultLauncher<SharingUtils.SharingIntent> launcher)
  {
    beginSharing(activity, launcher);
    List<BookmarkCategory> categories = BookmarkManager.INSTANCE.getCategories();
    long[] categoryIds = new long[categories.size()];
    for (int i = 0; i < categories.size(); i++)
      categoryIds[i] = categories.get(i).getId();
    BookmarkManager.INSTANCE.prepareCategoriesForSharing(categoryIds, FileType.Kml);
  }

  private void beginSharing(@NonNull FragmentActivity activity,
                            @NonNull ActivityResultLauncher<SharingUtils.SharingIntent> launcher)
  {
    mActivity = activity;
    mLauncher = launcher;
    // Register lazily and keep it: the singleton lives for the whole app, and a single permanent
    // registration sidesteps add/remove races while a preparation is in flight.
    if (!mListenerRegistered)
    {
      BookmarkManager.INSTANCE.addSharingListener(this);
      mListenerRegistered = true;
    }
    showProgressDialog(activity);
  }

  private void showProgressDialog(@NonNull FragmentActivity activity)
  {
    mProgressDialog = new ProgressDialog(activity, R.style.MwmTheme_ProgressDialog);
    mProgressDialog.setMessage(activity.getString(R.string.please_wait));
    mProgressDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
    mProgressDialog.setIndeterminate(true);
    mProgressDialog.setCancelable(false);
    mProgressDialog.show();
  }

  @Override
  public void onPreparedFileForSharing(@NonNull BookmarkSharingResult result)
  {
    final FragmentActivity activity = mActivity;
    final ActivityResultLauncher<SharingUtils.SharingIntent> launcher = mLauncher;
    mActivity = null;
    mLauncher = null;

    if (mProgressDialog != null && mProgressDialog.isShowing())
      mProgressDialog.dismiss();
    mProgressDialog = null;

    // The requester is gone (e.g. activity destroyed during preparation); drop the result silently.
    if (activity == null || launcher == null || activity.isDestroyed())
      return;

    switch (result.getCode())
    {
    case BookmarkSharingResult.SUCCESS ->
      SharingUtils.shareBookmarkFile(activity, launcher, result.getSharingPath(), result.getMimeType());
    case BookmarkSharingResult.EMPTY_CATEGORY ->
      new MaterialAlertDialogBuilder(activity, R.style.MwmTheme_AlertDialog)
          .setTitle(R.string.bookmarks_error_title_share_empty)
          .setMessage(R.string.bookmarks_error_message_share_empty)
          .setPositiveButton(R.string.ok, null)
          .show();
    case BookmarkSharingResult.ARCHIVE_ERROR, BookmarkSharingResult.FILE_ERROR ->
    {
      new MaterialAlertDialogBuilder(activity, R.style.MwmTheme_AlertDialog)
          .setTitle(R.string.dialog_routing_system_error)
          .setMessage(R.string.bookmarks_error_message_share_general)
          .setPositiveButton(R.string.ok, null)
          .show();
      List<String> names = new ArrayList<>();
      for (long categoryId : result.getCategoriesIds())
        names.add(BookmarkManager.INSTANCE.getCategoryById(categoryId).getName());
      Logger.e(TAG, "Failed to share bookmark categories " + names + ", error code: " + result.getCode());
    }
    default -> throw new AssertionError("Unsupported bookmark sharing code: " + result.getCode());
    }
  }
}
