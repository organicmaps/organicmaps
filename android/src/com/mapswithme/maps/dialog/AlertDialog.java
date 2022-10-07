package com.mapswithme.maps.dialog;

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.DrawableRes;
import androidx.annotation.LayoutRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.appcompat.app.AppCompatDialog;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.log.Logger;

public class AlertDialog extends BaseMwmDialogFragment
{
  private static final String TAG = AlertDialog.class.getSimpleName();

  private static final String ARG_TITLE_ID = "arg_title_id";
  private static final String ARG_MESSAGE_ID = "arg_message_id";
  private static final String ARG_POSITIVE_BUTTON_ID = "arg_positive_button_id";
  private static final String ARG_NEGATIVE_BUTTON_ID = "arg_negative_button_id";
  private static final String ARG_IMAGE_RES_ID = "arg_image_res_id";
  private static final String ARG_NEGATIVE_BTN_TEXT_COLOR_RES_ID = "arg_neg_btn_text_color_res_id";
  private static final String ARG_REQ_CODE = "arg_req_code";
  private static final String ARG_FRAGMENT_MANAGER_STRATEGY_INDEX = "arg_fragment_manager_strategy_index";
  private static final String ARG_DIALOG_VIEW_STRATEGY_INDEX = "arg_dialog_view_strategy_index";

  private static final int INVALID_ID = -1;

  @Nullable
  private AlertDialogCallback mTargetCallback;

  @NonNull
  private ResolveFragmentManagerStrategy mFragmentManagerStrategy = new ChildFragmentManagerStrategy();

  @NonNull
  private ResolveDialogViewStrategy mDialogViewStrategy = new AlertDialogStrategy();

  public void show(@NonNull Fragment parent, @Nullable String tag)
  {
    FragmentManager fm = mFragmentManagerStrategy.resolve(parent);
    if (fm.findFragmentByTag(tag) != null)
      return;

    showInternal(tag, fm);
  }

  public void show(@NonNull FragmentActivity activity, @Nullable String tag)
  {
    FragmentManager fm = mFragmentManagerStrategy.resolve(activity);
    if (fm.findFragmentByTag(tag) != null)
      return;

    showInternal(tag, fm);
  }

  private void showInternal(@Nullable String tag, @NonNull FragmentManager fm)
  {
    FragmentTransaction transaction = fm.beginTransaction();
    transaction.add(this, tag);
    transaction.commitAllowingStateLoss();
  }

  @LayoutRes
  protected int getLayoutId()
  {
    throw new UnsupportedOperationException("By default, you " +
                                            "shouldn't implement this method." +
                                            " AlertDialog.Builder will do everything by itself. " +
                                            "But if you want to use this method, " +
                                            "you'll have to implement it");
  }

  @Override
  public void onAttach(Context context)
  {
    super.onAttach(context);
    try
    {
      onAttachInternal();
    }
    catch (ClassCastException e)
    {
      Logger.w(TAG, "Caller doesn't implement AlertDialogCallback interface.");
    }
  }

  private void onAttachInternal()
  {
    mTargetCallback = (AlertDialogCallback) (getParentFragment() == null ? getTargetFragment()
                                                                         : getParentFragment());
    if (mTargetCallback != null)
      return;

    if (!(requireActivity() instanceof AlertDialogCallback))
      return;

    mTargetCallback = (AlertDialogCallback) requireActivity();
  }

  @Override
  public void onDetach()
  {
    super.onDetach();
    mTargetCallback = null;
  }

  protected void setTargetCallback(@Nullable AlertDialogCallback targetCallback)
  {
    mTargetCallback = targetCallback;
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    Bundle args = getArguments();
    if (args == null)
      throw new IllegalArgumentException("Arguments must be non null!");

    initStrategies(args);
    return mDialogViewStrategy.createView(this, args);
  }

  private void initStrategies(@NonNull Bundle args)
  {
    int fragManagerStrategyIndex = args.getInt(ARG_DIALOG_VIEW_STRATEGY_INDEX, INVALID_ID);
    mFragmentManagerStrategy = FragManagerStrategyType.values()[fragManagerStrategyIndex].getValue();

    int dialogViewStrategyIndex = args.getInt(ARG_DIALOG_VIEW_STRATEGY_INDEX, INVALID_ID);
    mDialogViewStrategy = DialogViewStrategyType.values()[dialogViewStrategyIndex].getValue();
  }

  private void onPositiveClicked(int which)
  {
    if (mTargetCallback != null)
      mTargetCallback.onAlertDialogPositiveClick(getArguments().getInt(ARG_REQ_CODE), which);
    dismissAllowingStateLoss();
  }

  private void onNegativeClicked(int which)
  {
    if (mTargetCallback != null)
      mTargetCallback.onAlertDialogNegativeClick(getArguments().getInt(ARG_REQ_CODE), which);
    dismissAllowingStateLoss();
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
    args.putInt(ARG_IMAGE_RES_ID, builder.getImageResId());
    args.putInt(ARG_NEGATIVE_BTN_TEXT_COLOR_RES_ID, builder.getNegativeBtnTextColor());

    FragManagerStrategyType fragManagerStrategyType = builder.getFragManagerStrategyType();
    args.putInt(ARG_FRAGMENT_MANAGER_STRATEGY_INDEX, fragManagerStrategyType.ordinal());

    DialogViewStrategyType dialogViewStrategyType = builder.getDialogViewStrategyType();
    args.putInt(ARG_DIALOG_VIEW_STRATEGY_INDEX, dialogViewStrategyType.ordinal());

    AlertDialog dialog = builder.getDialogFactory().createDialog();
    dialog.setArguments(args);
    dialog.setFragmentManagerStrategy(fragManagerStrategyType.getValue());
    dialog.setDialogViewStrategy(dialogViewStrategyType.getValue());
    return dialog;
  }

  private void setFragmentManagerStrategy(@NonNull ResolveFragmentManagerStrategy strategy)
  {
    mFragmentManagerStrategy = strategy;
  }

  private void setDialogViewStrategy(@NonNull ResolveDialogViewStrategy strategy)
  {
    mDialogViewStrategy = strategy;
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
    private int mNegativeBtn = INVALID_ID;
    @DrawableRes
    private int mImageResId = INVALID_ID;
    @NonNull
    private FragManagerStrategyType mFragManagerStrategyType = FragManagerStrategyType.DEFAULT;
    @NonNull
    private DialogViewStrategyType mDialogViewStrategyType = DialogViewStrategyType.DEFAULT;

    @NonNull
    private DialogFactory mDialogFactory = new DefaultDialogFactory();

    private int mNegativeBtnTextColor = INVALID_ID;

    @NonNull
    public Builder setReqCode(int reqCode)
    {
      mReqCode = reqCode;
      return this;
    }

    @NonNull
    public Builder setNegativeBtnTextColor(int negativeBtnTextColor)
    {
      mNegativeBtnTextColor = negativeBtnTextColor;
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

    int getNegativeBtnTextColor()
    {
      return mNegativeBtnTextColor;
    }

    @NonNull
    public Builder setImageResId(@DrawableRes int imageResId)
    {
      mImageResId = imageResId;
      return this;
    }

    @DrawableRes
    int getImageResId()
    {
      return mImageResId;
    }

    @NonNull
    public Builder setFragManagerStrategyType(@NonNull FragManagerStrategyType strategyType)
    {
      mFragManagerStrategyType = strategyType;
      return this;
    }

    @NonNull
    public Builder setDialogViewStrategyType(@NonNull DialogViewStrategyType strategyType)
    {
      mDialogViewStrategyType = strategyType;
      return this;
    }

    @NonNull
    DialogViewStrategyType getDialogViewStrategyType()
    {
      return mDialogViewStrategyType;
    }

    @NonNull
    FragManagerStrategyType getFragManagerStrategyType()
    {
      return mFragManagerStrategyType;
    }

    @NonNull
    public AlertDialog build()
    {
      return createDialog(this);
    }

    @NonNull
    public Builder setDialogFactory(@NonNull DialogFactory dialogFactory)
    {
      mDialogFactory = dialogFactory;
      return this;
    }

    @NonNull
    DialogFactory getDialogFactory()
    {
      return mDialogFactory;
    }
  }

  private static class ChildFragmentManagerStrategy implements ResolveFragmentManagerStrategy
  {
    @NonNull
    @Override
    public FragmentManager resolve(@NonNull Fragment baseFragment)
    {
      return baseFragment.getChildFragmentManager();
    }

    @NonNull
    @Override
    public FragmentManager resolve(@NonNull FragmentActivity activity)
    {
      throw new UnsupportedOperationException("Not supported here!");
    }
  }

  private static class ActivityFragmentManagerStrategy implements ResolveFragmentManagerStrategy
  {
    @NonNull
    @Override
    public FragmentManager resolve(@NonNull Fragment baseFragment)
    {
      return baseFragment.requireActivity().getSupportFragmentManager();
    }

    @NonNull
    @Override
    public FragmentManager resolve(@NonNull FragmentActivity activity)
    {
      return activity.getSupportFragmentManager();
    }
  }

  private static class AlertDialogStrategy implements ResolveDialogViewStrategy
  {
    @NonNull
    @Override
    public Dialog createView(@NonNull AlertDialog instance, @NonNull Bundle args)
    {
      int titleId = args.getInt(ARG_TITLE_ID);
      int messageId = args.getInt(ARG_MESSAGE_ID);
      int positiveButtonId = args.getInt(ARG_POSITIVE_BUTTON_ID);
      int negativeButtonId = args.getInt(ARG_NEGATIVE_BUTTON_ID);
      androidx.appcompat.app.AlertDialog.Builder builder =
          DialogUtils.buildAlertDialog(instance.requireContext(), titleId, messageId);
      builder.setPositiveButton(positiveButtonId,
                                (dialog, which) -> instance.onPositiveClicked(which));
      if (negativeButtonId != INVALID_ID)
        builder.setNegativeButton(negativeButtonId,
                                  (dialog, which) -> instance.onNegativeClicked(which));

      return builder.show();
    }
  }

  private static class ConfirmationDialogStrategy implements ResolveDialogViewStrategy
  {
    @NonNull
    @Override
    public Dialog createView(@NonNull AlertDialog fragment, @NonNull Bundle args)
    {
      AppCompatDialog appCompatDialog = new AppCompatDialog(fragment.requireContext());
      LayoutInflater inflater = LayoutInflater.from(fragment.requireContext());
      View root = inflater.inflate(fragment.getLayoutId(), null, false);

      TextView declineBtn = root.findViewById(R.id.decline_btn);
      int declineBtnTextId = args.getInt(ARG_NEGATIVE_BUTTON_ID);
      if (declineBtnTextId != INVALID_ID)
      {
        declineBtn.setText(args.getInt(ARG_NEGATIVE_BUTTON_ID));
        declineBtn.setOnClickListener(
            v -> fragment.onNegativeClicked(DialogInterface.BUTTON_NEGATIVE));
      }
      else
      {
        UiUtils.hide(declineBtn);
      }

      TextView acceptBtn = root.findViewById(R.id.accept_btn);
      acceptBtn.setText(args.getInt(ARG_POSITIVE_BUTTON_ID));
      acceptBtn.setOnClickListener(
          v -> fragment.onPositiveClicked(DialogInterface.BUTTON_POSITIVE));

      TextView descriptionView = root.findViewById(R.id.description);
      descriptionView.setText(args.getInt(ARG_MESSAGE_ID));

      TextView titleView = root.findViewById(R.id.title);
      titleView.setText(args.getInt(ARG_TITLE_ID));

      ImageView imageView = root.findViewById(R.id.image);
      int imageResId = args.getInt(ARG_IMAGE_RES_ID);
      boolean hasImage = imageResId != INVALID_ID;

      imageView.setImageDrawable(hasImage ? fragment.getResources().getDrawable(imageResId)
                                          : null);

      int negativeBtnTextColor = args.getInt(ARG_NEGATIVE_BTN_TEXT_COLOR_RES_ID);
      boolean hasNegativeBtnCustomColor = negativeBtnTextColor != INVALID_ID;

      if (hasNegativeBtnCustomColor)
        declineBtn.setTextColor(fragment.getResources().getColor(negativeBtnTextColor));

      UiUtils.showIf(hasImage, imageView);
      appCompatDialog.setContentView(root);
      return appCompatDialog;
    }
  }

  public enum FragManagerStrategyType
  {
    DEFAULT(new ChildFragmentManagerStrategy()),
    ACTIVITY_FRAGMENT_MANAGER(new ActivityFragmentManagerStrategy());

    @NonNull
    private final ResolveFragmentManagerStrategy mStrategy;

    FragManagerStrategyType(@NonNull ResolveFragmentManagerStrategy strategy)
    {
      mStrategy = strategy;
    }

    @NonNull
    public ResolveFragmentManagerStrategy getValue()
    {
      return mStrategy;
    }
  }

  public enum DialogViewStrategyType
  {
    DEFAULT(new AlertDialogStrategy()),
    CONFIRMATION_DIALOG(new ConfirmationDialogStrategy());

    @NonNull
    private final ResolveDialogViewStrategy mStrategy;

    DialogViewStrategyType(@NonNull ResolveDialogViewStrategy strategy)
    {
      mStrategy = strategy;
    }

    @NonNull
    private ResolveDialogViewStrategy getValue()
    {
      return mStrategy;
    }
  }
}
