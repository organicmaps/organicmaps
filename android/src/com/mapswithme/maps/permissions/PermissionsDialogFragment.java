package com.mapswithme.maps.permissions;

import android.content.DialogInterface;
import androidx.annotation.DrawableRes;
import androidx.annotation.IdRes;
import androidx.annotation.LayoutRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentManager;

import com.mapswithme.maps.R;

public class PermissionsDialogFragment extends BasePermissionsDialogFragment
{
  @Nullable
  public static DialogFragment show(@NonNull FragmentActivity activity, int requestCode)
  {
    return BasePermissionsDialogFragment.show(activity, requestCode, PermissionsDialogFragment.class);
  }

  public static DialogFragment find(@NonNull FragmentActivity activity)
  {
    final FragmentManager fm = activity.getSupportFragmentManager();
    if (fm.isDestroyed())
      return null;

    Fragment f = fm.findFragmentByTag(PermissionsDialogFragment.class.getName());
    return (DialogFragment) f;
  }

  @DrawableRes
  @Override
  protected int getImageRes()
  {
    return R.drawable.img_permissions;
  }

  @StringRes
  @Override
  protected int getTitleRes()
  {
    return R.string.onboarding_permissions_title;
  }

  @StringRes
  @Override
  protected int getSubtitleRes()
  {
    return R.string.onboarding_permissions_message;
  }

  @LayoutRes
  @Override
  protected int getLayoutRes()
  {
    return R.layout.fragment_permissions;
  }

  @IdRes
  @Override
  protected int getFirstActionButton()
  {
    return R.id.accept_btn;
  }

  @Override
  protected void onFirstActionClick()
  {
    PermissionsDetailDialogFragment.show(getActivity(), getRequestCode());
  }

  @IdRes
  @Override
  protected int getContinueActionButton()
  {
    return R.id.btn__continue;
  }

  @Override
  public void dismiss()
  {
    DialogFragment dialog = PermissionsDetailDialogFragment.find(getActivity());
    if (dialog != null)
      dialog.dismiss();
    super.dismiss();
  }

  @Override
  public void onCancel(DialogInterface dialog)
  {
    super.onCancel(dialog);
    getActivity().finish();
  }
}
