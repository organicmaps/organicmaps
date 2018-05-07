package com.mapswithme.util;

import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.support.annotation.NonNull;
import android.support.annotation.StringRes;
import android.support.v7.app.AlertDialog;

import com.mapswithme.maps.R;

public class DialogUtils
{
  private DialogUtils()
  {
  }

  private static AlertDialog.Builder buildAlertDialog(@NonNull Context context, @StringRes int titleId)
  {
    return new AlertDialog.Builder(context)
            .setCancelable(false)
            .setTitle(titleId)
            .setPositiveButton(R.string.ok, (dlg, which) -> dlg.dismiss());
  }

  private static AlertDialog.Builder buildAlertDialog(@NonNull Context context, @StringRes int titleId,
                                                      @StringRes int msgId)
  {
    return buildAlertDialog(context, titleId)
          .setMessage(msgId);
  }

  private static AlertDialog.Builder buildAlertDialog(@NonNull Context context, @StringRes int titleId,
                                                      @NonNull CharSequence msg,
                                                      @StringRes int posBtn,
                                                      @NonNull DialogInterface.OnClickListener posClickListener,
                                                      @StringRes int negBtn)
  {
    return buildAlertDialog(context, titleId)
        .setMessage(msg)
        .setPositiveButton(posBtn, posClickListener)
        .setNegativeButton(negBtn, null);
  }

  public static void showAlertDialog(@NonNull Context context, @StringRes int titleId,
                                     @StringRes int msgId)
  {
    buildAlertDialog(context, titleId, msgId).show();
  }

  public static void showAlertDialog(@NonNull Context context, @StringRes int titleId)
  {
    buildAlertDialog(context, titleId).show();
  }

  public static void showAlertDialog(@NonNull Context context, @StringRes int titleId,
                                     @NonNull CharSequence msg, @StringRes int posBtn,
                                     @NonNull DialogInterface.OnClickListener posClickListener,
                                     @StringRes int negBtn)
  {
    buildAlertDialog(context, titleId, msg, posBtn, posClickListener, negBtn).show();
  }

  @NonNull
  public static ProgressDialog createModalProgressDialog(@NonNull Context context, @StringRes int msg)
  {
    ProgressDialog progress = new ProgressDialog(context, R.style.MwmTheme_AlertDialog);
    progress.setMessage(context.getString(msg));
    progress.setProgressStyle(ProgressDialog.STYLE_SPINNER);
    progress.setIndeterminate(true);
    progress.setCancelable(false);
    return progress;
  }
}
