package app.organicmaps.base;

import android.os.Bundle;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.FragmentManager;

import app.organicmaps.R;
import app.organicmaps.dialog.ProgressDialogFragment;
import app.organicmaps.widget.ToolbarController;

public class BaseMwmToolbarFragment extends BaseMwmFragment
{
  @SuppressWarnings("NullableProblems")
  @NonNull
  private ToolbarController mToolbarController;

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    mToolbarController = onCreateToolbarController(view);
  }

  @NonNull
  protected ToolbarController onCreateToolbarController(@NonNull View root)
  {
    return new ToolbarController(root, requireActivity());
  }

  @NonNull
  protected ToolbarController getToolbarController()
  {
    return mToolbarController;
  }
}
