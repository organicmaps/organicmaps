package com.mapswithme.maps.permissions;

import android.app.Dialog;
import android.os.Bundle;
import android.support.annotation.IdRes;
import android.support.annotation.LayoutRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.FragmentActivity;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;

import com.mapswithme.maps.R;

public class PermissionsDetailDialogFragment extends BasePermissionsDialogFragment
{
  @Nullable
  public static DialogFragment show(@NonNull FragmentActivity activity, int requestId)
  {
    DialogFragment dialog = BasePermissionsDialogFragment.show(activity, requestId,
                                       PermissionsDetailDialogFragment.class);
    if (dialog != null)
      dialog.setCancelable(true);
    return dialog;
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
    return R.id.btn__back;
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
    return R.id.btn__continue;
  }
}
