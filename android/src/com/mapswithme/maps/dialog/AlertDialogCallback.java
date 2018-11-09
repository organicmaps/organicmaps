package com.mapswithme.maps.dialog;

public interface AlertDialogCallback
{
  /**
   *
   * @param requestCode the code that the dialog was launched with.
   * @param which indicates which button was pressed, for details see
   * {@link android.content.DialogInterface}
   *
   * @see android.content.DialogInterface.OnClickListener
   */
  void onAlertDialogPositiveClick(int requestCode, int which);

  /**
   *
   * @param requestCode the code that the dialog was launched with.
   * @param which indicates which button was pressed, for details see
   * {@link android.content.DialogInterface}
   *
   * @see android.content.DialogInterface.OnClickListener
   */
  void onAlertDialogNegativeClick(int requestCode, int which);

  /**
   * Called when the dialog is cancelled.
   *
   * @param requestCode the code that the dialog was launched with.
   */
  void onAlertDialogCancel(int requestCode);
}
