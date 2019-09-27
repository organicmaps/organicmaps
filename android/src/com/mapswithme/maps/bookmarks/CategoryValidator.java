package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.text.TextUtils;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.dialog.EditTextDialogFragment;
import com.mapswithme.maps.dialog.DialogUtils;

class CategoryValidator implements EditTextDialogFragment.Validator
{
  @Override
  public boolean validate(@NonNull Activity activity, @Nullable String text)
  {
    if (TextUtils.isEmpty(text))
    {
      DialogUtils.showAlertDialog(activity, R.string.bookmarks_error_title_empty_list_name,
                                  R.string.bookmarks_error_message_empty_list_name);
      return false;
    }

    if (BookmarkManager.INSTANCE.isUsedCategoryName(text))
    {
      DialogUtils.showAlertDialog(activity, R.string.bookmarks_error_title_list_name_already_taken,
                                  R.string.bookmarks_error_message_list_name_already_taken);
      return false;
    }

    return true;
  }
}
