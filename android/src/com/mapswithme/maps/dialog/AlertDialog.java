package com.mapswithme.maps.dialog;

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;

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

  public void show(@NonNull Fragment parent, @NonNull String tag)
  {
    FragmentManager fm = parent.getChildFragmentManager();
    if (fm.findFragmentByTag(tag) != null)
      return;

    FragmentTransaction transaction = fm.beginTransaction();
    transaction.add(this, tag);
    transaction.commitAllowingStateLoss();
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

  @NonNull
  private static AlertDialog createDialog(@NonNull Builder builder)
  {
    Bundle args = new Bundle();
    args.putInt(ARG_TITLE_ID, builder.getTitleId());
    args.putInt(ARG_MESSAGE_ID, builder.getMessageId());
    args.putInt(ARG_POSITIVE_BUTTON_ID, builder.getPositiveBtnId());
    args.putInt(ARG_REQ_CODE, builder.getReqCode());
    AlertDialog dialog = new AlertDialog();
    dialog.setArguments(args);
    return dialog;
  }

  public static class Builder
  {
    private int mReqCode;
    @StringRes
    private int mTitleId;
    @StringRes
    private int mMessageId;
    @StringRes
    private int mPositiveBtn;

    @NonNull
    public Builder setReqCode(int reqCode)
    {
      mReqCode = reqCode;
      return this;
    }

    int getReqCode()
    {
      return mReqCode;
    }

    @NonNull
    public Builder setTitleId(@StringRes int titleId)
    {
      mTitleId = titleId;
      return this;
    }

    @StringRes
    int getTitleId()
    {
      return mTitleId;
    }

    @NonNull
    public Builder setMessageId(@StringRes int messageId)
    {
      mMessageId = messageId;
      return this;
    }

    @StringRes
    int getMessageId()
    {
      return mMessageId;
    }

    @NonNull
    public Builder setPositiveBtnId(@StringRes int positiveBtnId)
    {
      mPositiveBtn = positiveBtnId;
      return this;
    }

    @StringRes
    int getPositiveBtnId()
    {
      return mPositiveBtn;
    }

    @NonNull
    public AlertDialog build()
    {
      return createDialog(this);
    }
  }
}
