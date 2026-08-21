package app.organicmaps.editor;

import android.app.Activity;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.fragment.app.FragmentManager;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmDialogFragment;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;

public final class InvalidFeaturePositionDialogFragment extends BaseMwmDialogFragment
{
  private static final String TAG = InvalidFeaturePositionDialogFragment.class.getSimpleName();

  public static void show(@NonNull FragmentManager manager)
  {
    if (manager.findFragmentByTag(TAG) == null)
      new InvalidFeaturePositionDialogFragment().show(manager, TAG);
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    return new MaterialAlertDialogBuilder(requireActivity(), R.style.MwmTheme_AlertDialog)
        .setTitle(R.string.message_invalid_feature_position)
        .setPositiveButton(R.string.ok, null)
        .create();
  }

  @Override
  public void onDismiss(@NonNull DialogInterface dialog)
  {
    super.onDismiss(dialog);
    final Activity activity = getActivity();
    if (activity != null)
      activity.finish();
  }
}
