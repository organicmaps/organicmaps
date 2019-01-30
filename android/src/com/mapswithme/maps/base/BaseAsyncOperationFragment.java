package com.mapswithme.maps.base;

import android.support.annotation.StringRes;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.FragmentManager;

import com.mapswithme.maps.R;
import com.mapswithme.maps.dialog.ProgressDialogFragment;

public abstract class BaseAsyncOperationFragment extends BaseMwmFragment
{
  private static final String PROGRESS_DIALOG_TAG = "base_progress_dialog";

  @SuppressWarnings("NullableProblems")
  protected void showProgress()
  {
    int resId = getProgressDialogTitle();
    String title = getString(resId);
    ProgressDialogFragment dialog = ProgressDialogFragment.newInstance(title);
    getFragmentManager()
        .beginTransaction()
        .add(dialog, PROGRESS_DIALOG_TAG)
        .commitAllowingStateLoss();
  }

  @StringRes
  protected int getProgressDialogTitle()
  {
    return R.string.downloading;
  }

  @SuppressWarnings("NullableProblems")
  protected void hideProgress()
  {
    FragmentManager fm = getFragmentManager();
    DialogFragment frag = (DialogFragment) fm.findFragmentByTag(PROGRESS_DIALOG_TAG);
    if (frag != null)
      frag.dismissAllowingStateLoss();
  }
}
