package com.mapswithme.maps.dialog;

import android.support.annotation.NonNull;

class DefaultDialogFactory implements DialogFactory
{
  @NonNull
  @Override
  public AlertDialog createDialog()
  {
    return new AlertDialog();
  }
}
