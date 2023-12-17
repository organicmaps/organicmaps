package app.organicmaps.dialog;

import android.app.Activity;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.Nullable;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.Fragment;

import app.organicmaps.R;

public class ProgressDialogFragment extends DialogFragment
{
  private static final String ARG_MESSAGE = "title";
  private static final String ARG_CANCELABLE = "cancelable";
  private static final String ARG_RETAIN_INSTANCE = "retain_instance";

  public ProgressDialogFragment()
  {
    // Do nothing by default.
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

  @Override
  public void onDestroyView()
  {
    if (getDialog() != null && getRetainInstance())
      getDialog().setDismissMessage(null);
    super.onDestroyView();
  }
}
