package com.mapswithme.maps.dialog;

import android.content.Context;

public class DrivingOptionsDialog extends AlertDialog
{
  @Override
  public void onAttach(Context context)
  {
    super.onAttach(context);
    AlertDialogCallback callback = (AlertDialogCallback) getActivity();
    setTargetCallback(callback);
  }
}
