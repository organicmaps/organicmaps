package com.mapswithme.maps.auth;

import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmExtraTitleActivity;
import com.mapswithme.maps.base.OnBackPressListener;

public class PhoneAuthActivity extends BaseMwmExtraTitleActivity
{
  public static void startForResult(@NonNull Fragment fragment)
  {
    final Intent i = new Intent(fragment.getContext(), PhoneAuthActivity.class);
    i.putExtra(EXTRA_TITLE, fragment.getString(R.string.authorization_button_sign_in));
    fragment.startActivityForResult(i, Constants.REQ_CODE_PHONE_AUTH_RESULT);
  }

  @Override
  public void onBackPressed()
  {
    FragmentManager manager = getSupportFragmentManager();
    Fragment fragment = manager.findFragmentByTag(PhoneAuthFragment.class.getName());

    if (fragment == null)
      return;

    if (!((OnBackPressListener) fragment).onBackPressed())
      super.onBackPressed();
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return PhoneAuthFragment.class;
  }
}
