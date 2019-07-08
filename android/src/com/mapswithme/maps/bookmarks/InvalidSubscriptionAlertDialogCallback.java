package com.mapswithme.maps.bookmarks;

import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.dialog.AlertDialogCallback;
import com.mapswithme.maps.purchase.BookmarkSubscriptionActivity;

class InvalidSubscriptionAlertDialogCallback implements AlertDialogCallback
{
  @NonNull
  private final Fragment mFragment;

  public InvalidSubscriptionAlertDialogCallback(@NonNull Fragment fragment)
  {
    mFragment = fragment;
  }

  @Override
  public void onAlertDialogPositiveClick(int requestCode, int which)
  {
    Intent intent = new Intent(mFragment.requireActivity(),
                               BookmarkSubscriptionActivity.class);
    mFragment.startActivityForResult(intent, BookmarksDownloadFragmentDelegate.REQ_CODE_SUBSCRIPTION_ACTIVITY);
  }

  @Override
  public void onAlertDialogNegativeClick(int requestCode, int which)
  {
    BookmarkManager.INSTANCE.deleteInvalidCategories();
  }

  @Override
  public void onAlertDialogCancel(int requestCode)
  {
  }
}
