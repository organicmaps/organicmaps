package com.mapswithme.maps.dialog;

public class ActivityCallbackAlertDialog extends AlertDialog
{
  @Override
  protected void onAttachInternal()
  {
    AlertDialogCallback callback = (AlertDialogCallback) getActivity();
    setTargetCallback(callback);
  }
}
