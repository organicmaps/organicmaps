package com.mapswithme.maps.bookmarks;

import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.dialog.AlertDialogCallback;
import com.mapswithme.maps.purchase.BookmarksAllSubscriptionActivity;
import com.mapswithme.maps.purchase.PurchaseUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;

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
    BookmarksAllSubscriptionActivity.startForResult(mFragment,
                                                    PurchaseUtils.REQ_CODE_PAY_CONTINUE_SUBSCRIPTION,
                                                    Statistics.ParamValue.POPUP);
  }

  @Override
  public void onAlertDialogNegativeClick(int requestCode, int which)
  {
    Logger logger = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
    String tag = InvalidSubscriptionAlertDialogCallback.class.getSimpleName();
    logger.i(tag, "Delete invalid categories, user didn't continue subscription...");
    BookmarkManager.INSTANCE.deleteExpiredCategories();
  }

  @Override
  public void onAlertDialogCancel(int requestCode)
  {
    // Invalid subs dialog is not cancellable, so do nothing here.
  }
}
