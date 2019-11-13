package com.mapswithme.maps.permissions;

import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.provider.Settings;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentManager;
import android.view.View;
import android.view.Window;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.SplashActivity;
import com.mapswithme.maps.base.BaseMwmDialogFragment;

public class StoragePermissionsDialogFragment extends BaseMwmDialogFragment implements View.OnClickListener
{
  @Nullable
  public static DialogFragment show(@NonNull FragmentActivity activity)
  {
    final FragmentManager fm = activity.getSupportFragmentManager();
    if (fm.isDestroyed())
      return null;

    Fragment f = fm.findFragmentByTag(StoragePermissionsDialogFragment.class.getName());
    if (f != null)
      return (DialogFragment) f;

    StoragePermissionsDialogFragment dialog = new StoragePermissionsDialogFragment();
    dialog.show(fm, StoragePermissionsDialogFragment.class.getName());

    return dialog;
  }

  public static DialogFragment find(SplashActivity activity)
  {
    final FragmentManager fm = activity.getSupportFragmentManager();
    if (fm.isDestroyed())
      return null;

    Fragment f = fm.findFragmentByTag(StoragePermissionsDialogFragment.class.getName());
    return (DialogFragment) f;
  }

  @Override
  protected int getCustomTheme()
  {
    // We can't read actual theme, because permissions are not granted yet.
    return R.style.MwmTheme_DialogFragment_Fullscreen;
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    Dialog res = super.onCreateDialog(savedInstanceState);
    res.requestWindowFeature(Window.FEATURE_NO_TITLE);

    View content = View.inflate(getActivity(), R.layout.fragment_storage_permissions, null);
    res.setContentView(content);

    TextView acceptBtn = content.findViewById(R.id.accept_btn);
    acceptBtn.setOnClickListener(this);
    acceptBtn.setText(R.string.settings);

    TextView declineBtn = content.findViewById(R.id.decline_btn);
    declineBtn.setOnClickListener(this);
    declineBtn.setText(R.string.back);

    ImageView image = (ImageView) content.findViewById(R.id.iv__image);
    image.setImageResource(R.drawable.img_no_storage_permission);
    TextView title = (TextView) content.findViewById(R.id.tv__title);
    title.setText(R.string.onboarding_storage_permissions_title);
    TextView subtitle = (TextView) content.findViewById(R.id.tv__subtitle1);
    subtitle.setText(R.string.onboarding_storage_permissions_message);

    return res;
  }

  @Override
  public void onClick(@NonNull View v)
  {
    switch (v.getId())
    {
      case R.id.decline_btn:
        getActivity().finish();
        break;

      case R.id.accept_btn:
        Intent intent = new Intent();
        intent.setAction(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
        Uri uri = Uri.fromParts("package", getContext().getPackageName(), null);
        intent.setData(uri);
        startActivity(intent);
        break;
    }
  }

  @Override
  public void onCancel(DialogInterface dialog)
  {
    super.onCancel(dialog);
    getActivity().finish();
  }
}
