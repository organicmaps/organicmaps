package com.mapswithme.maps.permissions;

import android.app.Dialog;
import android.os.Bundle;
import android.widget.TextView;

import androidx.annotation.IdRes;
import androidx.annotation.LayoutRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentManager;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import com.mapswithme.maps.R;

public class PermissionsDetailDialogFragment extends BasePermissionsDialogFragment
{
  @Nullable
  public static DialogFragment show(@NonNull FragmentActivity activity, int requestCode)
  {
    DialogFragment dialog = BasePermissionsDialogFragment.show(activity, requestCode,
                                       PermissionsDetailDialogFragment.class);
    if (dialog != null)
      dialog.setCancelable(true);
    return dialog;
  }

  @Nullable
  public static DialogFragment find(@NonNull FragmentActivity activity)
  {
    final FragmentManager fm = activity.getSupportFragmentManager();
    if (fm.isDestroyed())
      return null;

    Fragment f = fm.findFragmentByTag(PermissionsDetailDialogFragment.class.getName());
    return (DialogFragment) f;
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    Dialog res = super.onCreateDialog(savedInstanceState);
    RecyclerView permissions = (RecyclerView) res.findViewById(R.id.rv__permissions);
    permissions.setLayoutManager(new LinearLayoutManager(getContext(),
                                                         LinearLayoutManager.VERTICAL, false));
    permissions.setAdapter(new PermissionsAdapter());
    TextView acceptBtn = res.findViewById(R.id.accept_btn);
    acceptBtn.setText(R.string.continue_download);
    TextView declineBtn = res.findViewById(R.id.decline_btn);
    declineBtn.setText(R.string.back);
    return res;
  }

  @LayoutRes
  @Override
  protected int getLayoutRes()
  {
    return R.layout.fragment_detail_permissions;
  }

  @IdRes
  @Override
  protected int getFirstActionButton()
  {
    return R.id.decline_btn;
  }

  @Override
  protected void onFirstActionClick()
  {
    dismiss();
  }

  @IdRes
  @Override
  protected int getContinueActionButton()
  {
    return R.id.accept_btn;
  }
}
