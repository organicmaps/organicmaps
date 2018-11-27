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
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

public class AlertDialog extends BaseMwmDialogFragment
{
  private static final String ARG_TITLE_ID = "arg_title_id";
  private static final String ARG_MESSAGE_ID = "arg_message_id";
  private static final String ARG_POSITIVE_BUTTON_ID = "arg_positive_button_id";
  private static final String ARG_NEGATIVE_BUTTON_ID = "arg_negative_button_id";
  private static final String ARG_REQ_CODE = "arg_req_code";
  private static final int UNDEFINED_BUTTON_ID = -1;

  @NonNull
  private final DialogInterface.OnClickListener mPositiveClickListener
      = (dialog, which) -> AlertDialog.this.onClick(which);

  @Nullable
  private AlertDialogCallback mTargetCallback;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private ResolveFragmentManagerStrategy mFragmentManagerStrategy = new ChildFragmentManagerStrategy();

  public void show(@NonNull Fragment parent, @NonNull String tag)
  {
    FragmentManager fm = mFragmentManagerStrategy.resolve(parent);
    if (fm.findFragmentByTag(tag) != null)
      return;

    FragmentTransaction transaction = fm.beginTransaction();
    transaction.add(this, tag);
    transaction.commitAllowingStateLoss();
  }

  private void onClick(int which)
  {
    if (mTargetCallback != null)
      mTargetCallback.onAlertDialogPositiveClick(getArguments().getInt(ARG_REQ_CODE), which);
  }

  @Override
  public void onAttach(Context context)
  {
    super.onAttach(context);
    try
    {
      mTargetCallback = (AlertDialogCallback) (getParentFragment() == null ? getTargetFragment()
                                                                           : getParentFragment());
    }
    catch (ClassCastException e)
    {
      Logger logger = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
      logger.w(AlertDialog.class.getSimpleName(),
               "Caller doesn't implement AlertDialogCallback interface.");
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
    int negativeButtonId = args.getInt(ARG_NEGATIVE_BUTTON_ID);
    android.support.v7.app.AlertDialog.Builder builder =
        DialogUtils.buildAlertDialog(getContext(), titleId, messageId);
    builder.setPositiveButton(positiveButtonId, mPositiveClickListener);
    if (negativeButtonId != UNDEFINED_BUTTON_ID)
      builder.setNegativeButton(negativeButtonId, (dialog, which) -> onNegativeClicked(which));

    return builder.show();
  }

  private void onNegativeClicked(int which)
  {
    if (mTargetCallback != null)
      mTargetCallback.onAlertDialogNegativeClick(getArguments().getInt(ARG_REQ_CODE), which);
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
    args.putInt(ARG_NEGATIVE_BUTTON_ID, builder.getNegativeBtnId());
    args.putInt(ARG_REQ_CODE, builder.getReqCode());
    AlertDialog dialog = new AlertDialog();
    dialog.setArguments(args);
    dialog.setFragmentManagerStrategy(builder.getFragManagerStrategy());
    return dialog;
  }

  private void setFragmentManagerStrategy(@NonNull ResolveFragmentManagerStrategy strategy)
  {
    mFragmentManagerStrategy = strategy;
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
    @StringRes
    private int mNegativeBtn = UNDEFINED_BUTTON_ID;
    @NonNull
    private ResolveFragmentManagerStrategy mFragManagerStrategy = new ChildFragmentManagerStrategy();

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
    public Builder setPositiveBtnId(@StringRes int btnId)
    {
      mPositiveBtn = btnId;
      return this;
    }

    @NonNull
    public Builder setNegativeBtnId(@StringRes int btnId)
    {
      mNegativeBtn = btnId;
      return this;
    }

    @StringRes
    int getPositiveBtnId()
    {
      return mPositiveBtn;
    }

    @StringRes
    int getNegativeBtnId()
    {
      return mNegativeBtn;
    }

    @NonNull
    public Builder setFragManagerStrategy(@NonNull ResolveFragmentManagerStrategy fragManagerStrategy)
    {
      mFragManagerStrategy = fragManagerStrategy;
      return this;
    }

    @NonNull
    ResolveFragmentManagerStrategy getFragManagerStrategy()
    {
      return mFragManagerStrategy;
    }

    @NonNull
    public AlertDialog build()
    {
      return createDialog(this);
    }
  }

  public static class ChildFragmentManagerStrategy implements ResolveFragmentManagerStrategy
  {
    @NonNull
    @Override
    public FragmentManager resolve(@NonNull Fragment baseFragment)
    {
      return baseFragment.getChildFragmentManager();
    }
  }

  public static class ActivityFragmentManagerStrategy implements ResolveFragmentManagerStrategy
  {
    @NonNull
    @Override
    public FragmentManager resolve(@NonNull Fragment baseFragment)
    {
      return baseFragment.getActivity().getSupportFragmentManager();
    }
  }
}
