package com.mapswithme.maps.editor;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;

import androidx.annotation.IdRes;
import androidx.annotation.Nullable;
import androidx.annotation.Size;
import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.Fragment;

import com.mapswithme.maps.R;
import com.mapswithme.util.Constants;
import com.mapswithme.util.Utils;

public abstract class OsmAuthFragmentDelegate implements View.OnClickListener
{
  private final Fragment mFragment;

  protected abstract void loginOsm();

  public OsmAuthFragmentDelegate(Fragment fragment)
  {
    mFragment = fragment;
  }

  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    for (@IdRes int childId : new int[] {R.id.login_osm, R.id.register})
    {
      final View v = view.findViewById(childId);
      if (v != null)
        v.setOnClickListener(this);
    }
  }

  @Override
  public void onClick(View v)
  {
    // TODO show/hide spinners
    switch (v.getId())
    {
    case R.id.login_osm:
      loginOsm();
      break;
    case R.id.register:
      register();
      break;
    }
  }

  protected void processAuth(@Size(2) String[] auth, OsmOAuth.AuthType type, String username)
  {
    if (auth == null)
    {
      if (mFragment.isAdded())
      {
        new AlertDialog.Builder(mFragment.getActivity()).setTitle(R.string.editor_login_error_dialog)
                                                        .setPositiveButton(android.R.string.ok, null).show();
      }
      return;
    }

    OsmOAuth.setAuthorization(mFragment.requireContext(), auth[0], auth[1], username);
    if (mFragment.isAdded())
      Utils.navigateToParent(mFragment.getActivity());
  }

  protected void register()
  {
    mFragment.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.OSM_REGISTER)));
  }
}
