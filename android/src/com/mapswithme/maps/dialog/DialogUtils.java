package com.mapswithme.maps.dialog;

import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.appcompat.app.AlertDialog;

import com.mapswithme.maps.R;

public class DialogUtils
{
  private DialogUtils()
  {
  }

  @NonNull
  private static AlertDialog.Builder buildAlertDialog(@NonNull Context context, @StringRes int titleId)
  {
    return new AlertDialog.Builder(context)
            .setCancelable(false)
            .setTitle(titleId)
            .setPositiveButton(R.string.ok, (dlg, which) -> dlg.dismiss());
  }

  @NonNull
  static AlertDialog.Builder buildAlertDialog(@NonNull Context context, @StringRes int titleId,
                                                      @StringRes int msgId)
  {
    return buildAlertDialog(context, titleId)
          .setMessage(msgId);
  }

  @NonNull
  private static AlertDialog.Builder buildAlertDialog(@NonNull Context context, @StringRes int titleId,
                                                      @NonNull CharSequence msg, @StringRes int posBtn,
                                                      @NonNull DialogInterface.OnClickListener
                                                          posClickListener,
                                                      @StringRes int negBtn,
                                                      @Nullable DialogInterface.OnClickListener
                                                          negClickListener)
  {
    return buildAlertDialog(context, titleId, msg, posBtn, posClickListener)
        .setNegativeButton(negBtn, negClickListener);
  }

  @NonNull
  private static AlertDialog.Builder buildAlertDialog(@NonNull Context context, @StringRes int titleId,
                                                      @NonNull CharSequence msg, @StringRes int posBtn,
                                                      @NonNull DialogInterface.OnClickListener
                                                          posClickListener)
  {
    return buildAlertDialog(context, titleId)
        .setMessage(msg)
        .setPositiveButton(posBtn, posClickListener);
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
                                     @StringRes int msgId, @StringRes int posBtn,
                                     @NonNull DialogInterface.OnClickListener posClickListener,
                                     @StringRes int negBtn)
  {
    buildAlertDialog(context, titleId, context.getString(msgId), posBtn, posClickListener, negBtn,
                     null).show();
  }


  public static void showAlertDialog(@NonNull Context context, @StringRes int titleId,
                                     @NonNull CharSequence msg, @StringRes int posBtn,
                                     @NonNull DialogInterface.OnClickListener posClickListener,
                                     @StringRes int negBtn)
  {
    buildAlertDialog(context, titleId, msg, posBtn, posClickListener, negBtn, null).show();
  }

  public static void showAlertDialog(@NonNull Context context, @StringRes int titleId,
                                     @NonNull CharSequence msg, @StringRes int posBtn,
                                     @NonNull DialogInterface.OnClickListener posClickListener,
                                     @StringRes int negBtn,
                                     @Nullable DialogInterface.OnClickListener negClickListener)
  {
    buildAlertDialog(context, titleId, msg, posBtn, posClickListener, negBtn, negClickListener).show();
  }

  public static void showAlertDialog(@NonNull Context context, @StringRes int titleId,
                                     @StringRes int msgId, @StringRes int posBtn,
                                     @NonNull DialogInterface.OnClickListener posClickListener,
                                     @StringRes int negBtn,
                                     @Nullable DialogInterface.OnClickListener negClickListener)
  {
    buildAlertDialog(context, titleId, context.getString(msgId), posBtn, posClickListener, negBtn,
                     negClickListener).show();
  }

  public static void showAlertDialog(@NonNull Context context, @StringRes int titleId,
                                     @StringRes int msgId, @StringRes int posBtn,
                                     @NonNull DialogInterface.OnClickListener posClickListener)
  {
    buildAlertDialog(context, titleId, context.getString(msgId), posBtn, posClickListener).show();
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

  @NonNull
  public static ProgressDialog createModalProgressDialog(@NonNull Context context, @StringRes int msg,
                                                         int whichButton, @StringRes int buttonText,
                                                         @Nullable DialogInterface.OnClickListener
                                                             clickListener)
  {
    ProgressDialog progress = createModalProgressDialog(context, msg);
    progress.setButton(whichButton, context.getString(buttonText), clickListener);
    return progress;
  }
}
