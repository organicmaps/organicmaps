package com.mapswithme.util;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.annotation.StringRes;
import android.support.v7.app.AlertDialog;

import com.mapswithme.maps.R;

public class DialogUtils
{
  private DialogUtils()
  {
  }

  private static AlertDialog.Builder buildAlertDialog(@NonNull Activity activity, @StringRes int titleId)
  {
    return new AlertDialog.Builder(activity)
            .setCancelable(false)
            .setTitle(titleId)
            .setPositiveButton(R.string.ok, (dlg, which) -> dlg.dismiss());
  }

  private static AlertDialog.Builder buildAlertDialog(@NonNull Activity activity, @StringRes int titleId,
                                                      @StringRes int msgId)
  {
    return buildAlertDialog(activity, titleId)
          .setMessage(msgId);
  }

  private static AlertDialog.Builder buildAlertDialog(Activity activity, int titleId, @StringRes int msgId,
                                                      @StringRes int posBtn, @StringRes int negBtn)
  {
    return buildAlertDialog(activity, titleId, msgId)
        .setPositiveButton(posBtn, (dlg, which) -> dlg.dismiss())
        .setNegativeButton(negBtn, null);
  }

  public static void showAlertDialog(@NonNull Activity activity, @StringRes int titleId,
                                     @StringRes int msgId)
  {
    buildAlertDialog(activity, titleId, msgId).show();
  }

  public static void showAlertDialog(@NonNull Activity activity, @StringRes int titleId)
  {
    buildAlertDialog(activity, titleId).show();
  }

  public static void showAlertDialog(@NonNull Activity activity, @StringRes int titleId,
                                     @StringRes int msgId, @StringRes int posBtn, @StringRes int negBtn)
  {
    buildAlertDialog(activity, titleId, msgId, posBtn, negBtn).show();
  }
}
