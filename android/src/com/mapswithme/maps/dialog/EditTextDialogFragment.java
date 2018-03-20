package com.mapswithme.maps.dialog;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.design.widget.TextInputLayout;
import android.support.v4.app.Fragment;
import android.support.v7.app.AlertDialog;
import android.text.TextUtils;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.util.InputUtils;

public class EditTextDialogFragment extends BaseMwmDialogFragment
{
  public static final String EXTRA_TITLE = "DialogTitle";
  public static final String EXTRA_INITIAL = "InitialText";
  public static final String EXTRA_POSITIVE_BUTTON = "PositiveText";
  public static final String EXTRA_NEGATIVE_BUTTON = "NegativeText";
  public static final String EXTRA_HINT = "Hint";

  private String mTitle;
  @Nullable
  private String mInitialText;
  private String mHint;
  private EditText mEtInput;

  public interface OnTextSaveListener
  {
    void onSaveText(@Nullable String initialText, @Nullable String text);
  }

  public static void show(String title, @Nullable String initialText, String positiveBtn, String negativeBtn, Fragment parent)
  {
    show(title, initialText, "", positiveBtn, negativeBtn, parent);
  }

  public static void show(String title, String initialText, String hint, String positiveBtn, String negativeBtn, Fragment parent)
  {
    final Bundle args = new Bundle();
    args.putString(EXTRA_TITLE, title);
    args.putString(EXTRA_INITIAL, initialText);
    args.putString(EXTRA_POSITIVE_BUTTON, positiveBtn == null ? null : positiveBtn.toUpperCase());
    args.putString(EXTRA_NEGATIVE_BUTTON, negativeBtn == null ? null : negativeBtn.toUpperCase());
    args.putString(EXTRA_HINT, hint);
    final EditTextDialogFragment fragment = (EditTextDialogFragment) Fragment.instantiate(parent.getActivity(), EditTextDialogFragment.class.getName());
    fragment.setArguments(args);
    fragment.show(parent.getChildFragmentManager(), EditTextDialogFragment.class.getName());
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    final Bundle args = getArguments();
    String positiveButtonText = getString(R.string.ok);
    String negativeButtonText = getString(R.string.cancel);
    if (args != null)
    {
      mTitle = args.getString(EXTRA_TITLE);
      mInitialText = args.getString(EXTRA_INITIAL);
      mHint = args.getString(EXTRA_HINT);

      positiveButtonText = args.getString(EXTRA_POSITIVE_BUTTON);
      negativeButtonText = args.getString(EXTRA_NEGATIVE_BUTTON);
    }

    return new AlertDialog.Builder(getActivity())
                          .setView(buildView())
                          .setNegativeButton(negativeButtonText, null)
                          .setPositiveButton(positiveButtonText, new DialogInterface.OnClickListener()
                          {
                            @Override
                            public void onClick(DialogInterface dialog, int which)
                            {
                              final Fragment parentFragment = getParentFragment();
                              final String result = mEtInput.getText().toString();
                              if (parentFragment instanceof OnTextSaveListener)
                              {
                                dismiss();
                                ((OnTextSaveListener) parentFragment).onSaveText(mInitialText, result);
                                return;
                              }

                              final Activity activity = getActivity();
                              if (activity instanceof OnTextSaveListener)
                                ((OnTextSaveListener) activity).onSaveText(mInitialText, result);
                            }
                          }).create();
  }

  private View buildView()
  {
    @SuppressLint("InflateParams") final View root = getActivity().getLayoutInflater().inflate(R.layout.dialog_edit_text, null);
    TextInputLayout inputLayout = (TextInputLayout) root.findViewById(R.id.input);
    inputLayout.setHint(TextUtils.isEmpty(mHint) ? getString(R.string.name) : mHint);
    mEtInput = (EditText) inputLayout.findViewById(R.id.et__input);
    if (!TextUtils.isEmpty(mInitialText))
    {
      mEtInput.setText(mInitialText);
      mEtInput.selectAll();
    }

    InputUtils.showKeyboard(mEtInput);

    ((TextView) root.findViewById(R.id.tv__title)).setText(mTitle);
    return root;
  }
}
