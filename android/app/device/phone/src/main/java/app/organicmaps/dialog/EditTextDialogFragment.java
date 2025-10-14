package app.organicmaps.dialog;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.text.InputFilter;
import android.text.TextUtils;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentManager;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmDialogFragment;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.util.InputUtils;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

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
  private TextInputEditText mEtInput;
  private TextInputLayout mEtInputLayout;
  private Button mPositiveButton;
  private Validator mInputValidator;
  private OnTextSaveListener mTextSaveListener;

  // Interface of dialog input consumer.
  public interface OnTextSaveListener
  {
    void onSaveText(@NonNull String text);
  }

  // Interface of dialog input validator.
  public interface Validator
  {
    @Nullable
    String validate(@NonNull Activity activity, @Nullable String text);
  }

  public static EditTextDialogFragment show(@Nullable String title, @Nullable String initialText,
                                            @Nullable String positiveBtn, @Nullable String negativeBtn,
                                            @NonNull Fragment parent, @Nullable Validator inputValidator)
  {
    return show(title, initialText, "", positiveBtn, negativeBtn, NO_LIMITED_TEXT_LENGTH, parent, inputValidator);
  }

  public static EditTextDialogFragment show(@Nullable String title, @Nullable String initialText, @Nullable String hint,
                                            @Nullable String positiveBtn, @Nullable String negativeBtn,
                                            @NonNull Fragment parent, @Nullable Validator inputValidator)
  {
    return show(title, initialText, hint, positiveBtn, negativeBtn, NO_LIMITED_TEXT_LENGTH, parent, inputValidator);
  }

  public static EditTextDialogFragment show(@Nullable String title, @Nullable String initialText, @Nullable String hint,
                                            @Nullable String positiveBtn, @Nullable String negativeBtn, int textLimit,
                                            @NonNull Fragment parent, @Nullable Validator inputValidator)
  {
    final Bundle args = new Bundle();
    args.putString(ARG_TITLE, title);
    args.putString(ARG_INITIAL, initialText);
    args.putString(ARG_POSITIVE_BUTTON, positiveBtn);
    args.putString(ARG_NEGATIVE_BUTTON, negativeBtn);
    args.putString(ARG_HINT, hint);
    args.putInt(ARG_TEXT_LENGTH_LIMIT, textLimit);
    FragmentManager fragmentManager = parent.getChildFragmentManager();
    final EditTextDialogFragment fragment = (EditTextDialogFragment) fragmentManager.getFragmentFactory().instantiate(
        parent.requireActivity().getClassLoader(), EditTextDialogFragment.class.getName());
    fragment.setArguments(args);
    fragment.show(fragmentManager, EditTextDialogFragment.class.getName());
    fragment.mInputValidator = inputValidator;

    return fragment;
  }

  public void setTextSaveListener(OnTextSaveListener textSaveListener)
  {
    mTextSaveListener = textSaveListener;
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

    AlertDialog editTextDialog = new MaterialAlertDialogBuilder(requireActivity(), R.style.MwmTheme_AlertDialog)
                                     .setView(buildView())
                                     .setNegativeButton(negativeButtonText, null)
                                     .setPositiveButton(positiveButtonText,
                                                        (dialog, which) -> {
                                                          final String result = mEtInput.getText().toString();
                                                          processInput(result);
                                                          dismiss();
                                                        })
                                     .create();

    // Wait till alert is shown to get mPositiveButton.
    editTextDialog.setOnShowListener((dialog) -> {
      mPositiveButton = editTextDialog.getButton(DialogInterface.BUTTON_POSITIVE);
      final FragmentActivity activity = getActivity();
      if (activity == null)
        return;
      this.validateInput(activity, mInitialText);
    });

    // Setup validation on input edit.
    mEtInput.addTextChangedListener(new StringUtils.SimpleTextWatcher() {
      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count)
      {
        final FragmentActivity activity = getActivity();
        if (activity == null)
          return;
        EditTextDialogFragment.this.validateInput(activity, s.toString());
      }
    });

    return editTextDialog;
  }

  private void validateInput(@NonNull FragmentActivity activity, @Nullable String input)
  {
    if (mPositiveButton != null && mInputValidator != null)
    {
      final String maybeError = mInputValidator.validate(activity, input);
      mPositiveButton.setEnabled(maybeError == null);
      mEtInputLayout.getEditText().setError(maybeError);
    }
  }

  private void processInput(@Nullable String text)
  {
    if (mTextSaveListener != null)
    {
      if (TextUtils.isEmpty(text))
        throw new AssertionError("Input must be non-empty!");

      mTextSaveListener.onSaveText(text);
    }
  }

  private View buildView()
  {
    @SuppressLint("InflateParams")
    final View root = requireActivity().getLayoutInflater().inflate(R.layout.dialog_edit_text, null);
    mEtInputLayout = root.findViewById(R.id.et__input_layout);
    mEtInput = mEtInputLayout.findViewById(R.id.et__input);
    mEtInput.setHint(TextUtils.isEmpty(mHint) ? getString(R.string.name) : mHint);
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
