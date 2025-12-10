package app.organicmaps.editor;

import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.DialogFragment;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import app.organicmaps.R;

public class OsmLoginAuthFailDialogFragment extends DialogFragment
{
  public static OsmLoginAuthFailDialogFragment newInstance()
  {
    return new OsmLoginAuthFailDialogFragment();
  }

  @NonNull
  @Override
  public AlertDialog onCreateDialog(@Nullable Bundle savedInstanceState)
  {
    return new MaterialAlertDialogBuilder(requireActivity(), R.style.MwmTheme_AlertDialog)
        .setTitle(R.string.editor_login_error_dialog)
        .setPositiveButton(R.string.ok, null).create();
  }
}
