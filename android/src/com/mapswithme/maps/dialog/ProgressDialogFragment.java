package com.mapswithme.maps.dialog;

import android.app.Activity;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;
import android.support.v7.app.AlertDialog;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;

import com.mapswithme.maps.R;

import java.util.Objects;

public class ProgressDialogFragment extends DialogFragment
{
  private static final String EXTRA_TITLE = "title";
  private static final String EXTRA_CANCELABLE = "cancelable";
  private static final String EXTRA_RETAIN_INSTANCE = "retain_instance";

  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mMessageView;

  public ProgressDialogFragment()
  {
  }

  @NonNull
  public static ProgressDialogFragment newInstance(@NonNull String title)
  {
    return newInstance(title, false, true);
  }

  @NonNull
  public static ProgressDialogFragment newInstance(@NonNull String title, boolean cancelable,
                                                   boolean retainInstance)
  {
    ProgressDialogFragment fr = new ProgressDialogFragment();
    fr.setArguments(getArgs(title, cancelable, retainInstance));
    return fr;
  }

  private static Bundle getArgs(@NonNull String title, boolean cancelable, boolean retainInstance)
  {
    Bundle args = new Bundle();
    args.putString(EXTRA_TITLE, title);
    args.putBoolean(EXTRA_CANCELABLE, cancelable);
    args.putBoolean(EXTRA_RETAIN_INSTANCE, retainInstance);
    return args;
  }

  protected void setCancelResult()
  {
    Fragment targetFragment = getTargetFragment();
    if (targetFragment != null)
      targetFragment.onActivityResult(getTargetRequestCode(), Activity.RESULT_CANCELED, null);
  }

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    Bundle args = Objects.requireNonNull(getArguments());
    boolean retainInstance = args.getBoolean(EXTRA_RETAIN_INSTANCE, true);
    setRetainInstance(retainInstance);
  }

  @NonNull
  @Override
  public final Dialog onCreateDialog(Bundle savedInstanceState)
  {
    return onCreateProgressDialog();
  }

  @NonNull
  protected AlertDialog onCreateProgressDialog()
  {
    AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
    LayoutInflater inflater = LayoutInflater.from(getActivity());
    View view = inflater.inflate(R.layout.indeterminated_progress_dialog, null, false);
    Bundle args = Objects.requireNonNull(getArguments());
    String title = args.getString(EXTRA_TITLE);
    mMessageView.setText(title);
    mMessageView = view.findViewById(R.id.message);
    builder.setView(view);
    AlertDialog dialog = builder.create();
    dialog.setCanceledOnTouchOutside(args.getBoolean(EXTRA_CANCELABLE, false));
    return dialog;
  }

  @Override
  public void onCancel(DialogInterface dialog)
  {
    setCancelResult();
  }

  public boolean isShowing()
  {
    Dialog dialog = getDialog();
    return dialog != null && dialog.isShowing();
  }

  public void setMessage(@NonNull CharSequence message)
  {
    mMessageView.setText(message);
  }

  @Override
  public void onDestroyView()
  {
    if (getDialog() != null && getRetainInstance())
      getDialog().setDismissMessage(null);
    super.onDestroyView();
  }
}
