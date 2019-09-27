package com.mapswithme.maps.dialog;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Dialog;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.google.android.material.textfield.TextInputLayout;
import androidx.fragment.app.Fragment;
import androidx.appcompat.app.AlertDialog;
import android.text.InputFilter;
import android.text.TextUtils;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.util.InputUtils;

public class EditTextDialogFragment extends BaseMwmDialogFragment
{
  public static final String ARG_TITLE = "arg_dialog_title";
  public static final String ARG_INITIAL = "arg_initial";
  public static final String ARG_POSITIVE_BUTTON = "arg_positive_button";
  public static final String ARG_NEGATIVE_BUTTON = "arg_negative_button";
  public static final String ARG_HINT = "arg_hint";
  public static final String ARG_TEXT_LENGTH_LIMIT = "arg_text_length_limit";
  private static final int NO_LIMITED_TEXT_LENGTH = -1;

  private String mTitle;
  @Nullable
  private String mInitialText;
  private String mHint;
  private EditText mEtInput;

  public interface EditTextDialogInterface
  {
    @NonNull
    OnTextSaveListener getSaveTextListener();

    @NonNull
    Validator getValidator();
  }

  public interface OnTextSaveListener
  {
    void onSaveText(@NonNull String text);
  }

  public interface Validator
  {
    boolean validate(@NonNull Activity activity, @Nullable String text);
  }

  public static void show(@Nullable String title, @Nullable String initialText,
                          @Nullable String positiveBtn, @Nullable String negativeBtn,
                          @NonNull Fragment parent)
  {
    show(title, initialText, "", positiveBtn, negativeBtn, NO_LIMITED_TEXT_LENGTH, parent);
  }

  public static void show(@Nullable String title, @Nullable String initialText,
                          @Nullable String positiveBtn, @Nullable String negativeBtn,
                          int textLimit, @NonNull Fragment parent)
  {
    show(title, initialText, "", positiveBtn, negativeBtn, textLimit, parent);
  }

  public static void show(@Nullable String title, @Nullable String initialText, @Nullable String hint,
                          @Nullable String positiveBtn, @Nullable String negativeBtn,
                          @NonNull Fragment parent)
  {
    show(title, initialText, hint, positiveBtn, negativeBtn, NO_LIMITED_TEXT_LENGTH, parent);
  }

  public static void show(@Nullable String title, @Nullable String initialText, @Nullable String hint,
                          @Nullable String positiveBtn, @Nullable String negativeBtn, int textLimit,
                          @NonNull Fragment parent)
  {
    final Bundle args = new Bundle();
    args.putString(ARG_TITLE, title);
    args.putString(ARG_INITIAL, initialText);
    args.putString(ARG_POSITIVE_BUTTON, positiveBtn == null ? null : positiveBtn.toUpperCase());
    args.putString(ARG_NEGATIVE_BUTTON, negativeBtn == null ? null : negativeBtn.toUpperCase());
    args.putString(ARG_HINT, hint);
    args.putInt(ARG_TEXT_LENGTH_LIMIT, textLimit);
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
      mTitle = args.getString(ARG_TITLE);
      mInitialText = args.getString(ARG_INITIAL);
      mHint = args.getString(ARG_HINT);

      positiveButtonText = args.getString(ARG_POSITIVE_BUTTON);
      negativeButtonText = args.getString(ARG_NEGATIVE_BUTTON);
    }

    return new AlertDialog.Builder(getActivity())
        .setView(buildView())
        .setNegativeButton(negativeButtonText, null)
        .setPositiveButton(positiveButtonText, (dialog, which) -> {
          final Fragment parentFragment = getParentFragment();
          final String result = mEtInput.getText().toString();
          if (parentFragment instanceof EditTextDialogInterface)
          {
            dismiss();
            processInput((EditTextDialogInterface) parentFragment, result);
            return;
          }

          final Activity activity = getActivity();
          if (activity instanceof EditTextDialogInterface)
          {
            processInput((EditTextDialogInterface) activity, result);
          }
        }).create();
  }

  private void processInput(@NonNull EditTextDialogInterface editInterface,
                            @Nullable String text)
  {
    Validator validator = editInterface.getValidator();
    if (!validator.validate(getActivity(), text))
      return;

    if (TextUtils.isEmpty(text))
      throw new AssertionError("Input must be non-empty!");

    editInterface.getSaveTextListener().onSaveText(text);
  }

  private View buildView()
  {
    @SuppressLint("InflateParams") final View root = getActivity().getLayoutInflater().inflate(R.layout.dialog_edit_text, null);
    TextInputLayout inputLayout = root.findViewById(R.id.input);
    inputLayout.setHint(TextUtils.isEmpty(mHint) ? getString(R.string.name) : mHint);
    mEtInput = inputLayout.findViewById(R.id.et__input);
    int maxLength = getArguments().getInt(ARG_TEXT_LENGTH_LIMIT);
    if (maxLength != NO_LIMITED_TEXT_LENGTH)
    {
      InputFilter[] f = {new InputFilter.LengthFilter(maxLength)};
      mEtInput.setFilters(f);
    }

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
