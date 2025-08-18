package app.organicmaps.bookmarks;

import android.app.Activity;
import android.text.TextUtils;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.R;
import app.organicmaps.dialog.EditTextDialogFragment;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;

class CategoryValidator implements EditTextDialogFragment.Validator
{
  @Nullable
  @Override
  public String validate(@NonNull Activity activity, @Nullable String text)
  {
    if (TextUtils.isEmpty(text))
      return activity.getString(R.string.bookmarks_error_title_empty_list_name);

    if (BookmarkManager.INSTANCE.isUsedCategoryName(text))
      return activity.getString(R.string.bookmarks_error_title_list_name_already_taken);

    return null;
  }
}
