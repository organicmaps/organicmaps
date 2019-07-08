package com.mapswithme.maps.bookmarks;

import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.dialog.AlertDialogCallback;
import com.mapswithme.maps.purchase.BookmarkSubscriptionActivity;
import com.mapswithme.maps.purchase.PurchaseUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

class InvalidSubscriptionAlertDialogCallback implements AlertDialogCallback
{
  @NonNull
  private final Fragment mFragment;

  InvalidSubscriptionAlertDialogCallback(@NonNull Fragment fragment)
  {
    mFragment = fragment;
  }

  @Override
  public void onAlertDialogPositiveClick(int requestCode, int which)
  {
    BookmarkSubscriptionActivity.startForResult(mFragment, PurchaseUtils.REQ_CODE_PAY_CONTINUE_SUBSCRIPTION);
  }

  @Override
  public void onAlertDialogNegativeClick(int requestCode, int which)
  {
    Logger logger = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
    String tag = InvalidSubscriptionAlertDialogCallback.class.getSimpleName();
    logger.i(tag, "Delete invalid categories, user didn't continue subscription...");
    BookmarkManager.INSTANCE.deleteInvalidCategories();
  }

  @Override
  public void onAlertDialogCancel(int requestCode)
  {
  }
}
