package com.mapswithme.maps.dialog;

import android.app.Activity;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.Fragment;

import com.mapswithme.maps.R;

import java.util.Objects;

public class ProgressDialogFragment extends DialogFragment
{
  private static final String ARG_MESSAGE = "title";
  private static final String ARG_CANCELABLE = "cancelable";
  private static final String ARG_RETAIN_INSTANCE = "retain_instance";

  public ProgressDialogFragment()
  {
    // Do nothing by default.
  }

  @NonNull
  public static ProgressDialogFragment newInstance(@NonNull String message)
  {
    return newInstance(message, false, true);
  }

  @NonNull
  public static ProgressDialogFragment newInstance(@NonNull String message, boolean cancelable,
                                                   boolean retainInstance)
  {
    ProgressDialogFragment fr = new ProgressDialogFragment();
    fr.setArguments(getArgs(message, cancelable, retainInstance));
    return fr;
  }

  private static Bundle getArgs(@NonNull String title, boolean cancelable, boolean retainInstance)
  {
    Bundle args = new Bundle();
    args.putString(ARG_MESSAGE, title);
    args.putBoolean(ARG_CANCELABLE, cancelable);
    args.putBoolean(ARG_RETAIN_INSTANCE, retainInstance);
    return args;
  }

  protected void setCancelResult()
  {
    Fragment targetFragment = getTargetFragment();
    if (targetFragment != null)
      targetFragment.onActivityResult(getTargetRequestCode(), Activity.RESULT_CANCELED, null);
  }

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    Bundle args = requireArguments();
    setRetainInstance(args.getBoolean(ARG_RETAIN_INSTANCE, true));
    setCancelable(args.getBoolean(ARG_CANCELABLE, false));
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable
      Bundle savedInstanceState)
  {
    View view = inflater.inflate(R.layout.indeterminated_progress_dialog, container, false);
    Bundle args = requireArguments();
    TextView messageView = view.findViewById(R.id.message);
    messageView.setText(args.getString(ARG_MESSAGE));
    return view;
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

  @Override
  public void onDestroyView()
  {
    if (getDialog() != null && getRetainInstance())
      getDialog().setDismissMessage(null);
    super.onDestroyView();
  }
}
