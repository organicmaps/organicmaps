package com.mapswithme.maps.base;

import androidx.annotation.StringRes;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.FragmentManager;

import com.mapswithme.maps.R;
import com.mapswithme.maps.dialog.ProgressDialogFragment;

import java.util.Objects;

public abstract class BaseAsyncOperationFragment extends BaseMwmFragment
{
  private static final String PROGRESS_DIALOG_TAG = "base_progress_dialog";

  protected void showProgress()
  {
    int resId = getProgressMessageId();
    String title = getString(resId);
    ProgressDialogFragment dialog = ProgressDialogFragment.newInstance(title);
    Objects.requireNonNull(getFragmentManager())
           .beginTransaction()
           .add(dialog, PROGRESS_DIALOG_TAG)
           .commitAllowingStateLoss();
  }

  @StringRes
  protected int getProgressMessageId()
  {
    return R.string.downloading;
  }

  protected void hideProgress()
  {
    FragmentManager fm = getFragmentManager();
    DialogFragment frag = (DialogFragment) Objects.requireNonNull(fm)
                                                  .findFragmentByTag(PROGRESS_DIALOG_TAG);
    if (frag != null)
      frag.dismissAllowingStateLoss();
  }
}
