package com.mapswithme.maps.bookmarks;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.util.DialogUtils;

public class KmlImportController implements BookmarkManager.KmlConversionListener
{
  @NonNull
  private final Activity mContext;
  @Nullable
  private ProgressDialog mProgressDialog;
  @Nullable
  private final ImportKmlCallback mCallback;
  private boolean mWasShown = false;

  KmlImportController(@NonNull Activity context, @Nullable ImportKmlCallback callback)
  {
    mContext = context;
    mCallback = callback;
  }

  void onStart()
  {
    BookmarkManager.INSTANCE.addKmlConversionListener(this);
  }

  void onStop()
  {
    BookmarkManager.INSTANCE.removeKmlConversionListener(this);
  }

  void importKml()
  {
    if (mWasShown)
      return;

    if (BookmarkManager.INSTANCE.isAsyncBookmarksLoadingInProgress())
      return;

    int count = BookmarkManager.INSTANCE.getKmlFilesCountForConversion();
    if (count == 0)
      return;

    DialogInterface.OnClickListener clickListener = (dialog, which) -> {
      BookmarkManager.INSTANCE.convertAllKmlFiles();
      dialog.dismiss();
      mProgressDialog = DialogUtils.createModalProgressDialog(mContext, R.string.converting);
      mProgressDialog.show();
    };

    String msg = mContext.getString(R.string.bookmarks_detect_message, count);
    DialogUtils.showAlertDialog(mContext, R.string.bookmarks_detect_title, msg,
                                R.string.button_convert, clickListener, R.string.cancel);
    mWasShown = true;
  }

  @Override
  public void onFinishKmlConversion(boolean success)
  {
    if (mProgressDialog != null && mProgressDialog.isShowing())
      mProgressDialog.dismiss();

    if (success)
    {
      if (mCallback != null)
        mCallback.onFinishKmlImport();
      return;
    }

    DialogUtils.showAlertDialog(mContext, R.string.bookmarks_convert_error_title,
                                R.string.bookmarks_convert_error_message);
  }

  interface ImportKmlCallback
  {
    void onFinishKmlImport();
  }
}
