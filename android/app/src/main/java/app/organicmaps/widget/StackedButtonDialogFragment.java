package app.organicmaps.widget;

import android.app.Dialog;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;
import app.organicmaps.R;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.NetworkPolicy;

public class StackedButtonDialogFragment extends DialogFragment
{
  @Nullable
  private NetworkPolicy.NetworkPolicyListener mListener;

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    return new StackedButtonsDialog.Builder(requireContext())
        .setTitle(R.string.mobile_data_dialog)
        .setMessage(R.string.mobile_data_description)
        .setCancelable(false)
        .setPositiveButton(R.string.mobile_data_option_always,
                           (dialog, which) -> onDialogBtnClicked(NetworkPolicy.Type.ALWAYS, true))
        .setNegativeButton(R.string.mobile_data_option_not_today,
                           (dialog, which) -> onMobileDataImpactBtnClicked(NetworkPolicy.Type.NOT_TODAY, false))
        .setNeutralButton(R.string.mobile_data_option_today,
                          (dialog, which) -> onMobileDataImpactBtnClicked(NetworkPolicy.Type.TODAY, true))
        .build();
  }

  private void onMobileDataImpactBtnClicked(@NonNull NetworkPolicy.Type today, boolean canUse)
  {
    Config.setMobileDataTimeStamp(System.currentTimeMillis());
    onDialogBtnClicked(today, canUse);
  }

  private void onDialogBtnClicked(@NonNull NetworkPolicy.Type type, boolean canUse)
  {
    Config.setUseMobileDataSettings(type);
    if (mListener != null)
      mListener.onResult(NetworkPolicy.newInstance(canUse));
  }

  @Override
  public void show(@NonNull FragmentManager manager, @NonNull String tag)
  {
    FragmentTransaction ft = manager.beginTransaction();
    ft.add(this, tag);
    ft.commitAllowingStateLoss();
  }

  public void setListener(@Nullable NetworkPolicy.NetworkPolicyListener listener)
  {
    mListener = listener;
  }
}
