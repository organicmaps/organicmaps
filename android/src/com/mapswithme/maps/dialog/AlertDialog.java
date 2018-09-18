package com.mapswithme.maps.dialog;

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.base.BaseMwmDialogFragment;

public class AlertDialog extends BaseMwmDialogFragment
{
  private static final String ARG_TITLE_ID = "arg_title_id";
  private static final String ARG_MESSAGE_ID = "arg_message_id";
  private static final String ARG_POSITIVE_BUTTON_ID = "arg_positive_button_id";
  private static final String ARG_REQ_CODE = "arg_req_code";

  @NonNull
  private final DialogInterface.OnClickListener mPositiveClickListener
      = (dialog, which) -> AlertDialog.this.onClick(which);

  @Nullable
  private AlertDialogCallback mTargetCallback;

  public static void show(@StringRes int titleId, @StringRes int messageId,
                          @StringRes int positiveBtnId, @NonNull Fragment parent,
                          int requestCode)
  {
    Bundle args = new Bundle();
    args.putInt(ARG_TITLE_ID, titleId);
    args.putInt(ARG_MESSAGE_ID, messageId);
    args.putInt(ARG_POSITIVE_BUTTON_ID, positiveBtnId);
    args.putInt(ARG_REQ_CODE, requestCode);
    DialogFragment fragment = (DialogFragment) Fragment.instantiate(parent.getActivity(),
                                                                    AlertDialog.class.getName());
    fragment.setArguments(args);
    fragment.show(parent.getChildFragmentManager(), AlertDialog.class.getName());
  }

  private void onClick(int which)
  {
    if (mTargetCallback != null)
      mTargetCallback.onAlertDialogClick(getArguments().getInt(ARG_REQ_CODE), which);
  }

  @Override
  public void onAttach(Context context)
  {
    super.onAttach(context);
    try
    {
      mTargetCallback = (AlertDialogCallback) getParentFragment();
    }
    catch (ClassCastException e)
    {
      throw new ClassCastException("Caller must implement AlertDialogCallback interface!");
    }
  }

  @Override
  public void onDetach()
  {
    super.onDetach();
    mTargetCallback = null;
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    Bundle args = getArguments();
    if (args == null)
      throw new IllegalArgumentException("Arguments must be non null!");

    int titleId = args.getInt(ARG_TITLE_ID);
    int messageId = args.getInt(ARG_MESSAGE_ID);
    int positiveButtonId = args.getInt(ARG_POSITIVE_BUTTON_ID);
    android.support.v7.app.AlertDialog.Builder builder =
        DialogUtils.buildAlertDialog(getContext(), titleId, messageId);
    builder.setPositiveButton(positiveButtonId, mPositiveClickListener);
    return builder.show();
  }

  @Override
  public void onCancel(DialogInterface dialog)
  {
    super.onCancel(dialog);
    if (mTargetCallback != null)
      mTargetCallback.onAlertDialogCancel(getArguments().getInt(ARG_REQ_CODE));
  }
}
