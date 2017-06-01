package com.mapswithme.maps.permissions;

import android.os.Bundle;
import android.support.annotation.DrawableRes;
import android.support.annotation.IdRes;
import android.support.annotation.LayoutRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;

import com.mapswithme.maps.R;
import com.mapswithme.maps.SplashActivity;

public class PermissionsDialogFragment extends BasePermissionsDialogFragment
{
  @Nullable
  private DialogFragment mDetailDialog;

  @Nullable
  public static DialogFragment show(@NonNull FragmentActivity activity, int requestId)
  {
    return BasePermissionsDialogFragment.show(activity, requestId, PermissionsDialogFragment.class);
  }

  public static DialogFragment find(SplashActivity activity)
  {
    final FragmentManager fm = activity.getSupportFragmentManager();
    if (fm.isDestroyed())
      return null;

    Fragment f = fm.findFragmentByTag(PermissionsDialogFragment.class.getName());
    return (DialogFragment) f;
  }

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    Fragment f = getActivity().getSupportFragmentManager()
                              .findFragmentByTag(PermissionsDetailDialogFragment.class.getName());
    if (f != null)
      mDetailDialog = (DialogFragment) f;
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
    return R.id.btn__learn_more;
  }

  @Override
  protected void onFirstActionClick()
  {
    mDetailDialog = PermissionsDetailDialogFragment.show(getActivity(), getRequestId());
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
    if (mDetailDialog != null)
      mDetailDialog.dismiss();
    super.dismiss();
  }
}
