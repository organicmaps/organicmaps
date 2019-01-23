package com.mapswithme.maps.dialog;

import android.support.annotation.NonNull;

public class ConfirmationDialogFactory implements DialogFactory
{
  @NonNull
  @Override
  public AlertDialog createDialog()
  {
    return new DefaultConfirmationAlertDialog();
  }
}
