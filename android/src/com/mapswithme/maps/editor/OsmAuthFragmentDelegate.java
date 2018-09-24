package com.mapswithme.maps.editor;

import android.app.Application;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.IdRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.Size;
import android.support.v4.app.Fragment;
import android.support.v7.app.AlertDialog;
import android.view.View;

import com.mapswithme.maps.R;
import com.mapswithme.util.Constants;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;

public abstract class OsmAuthFragmentDelegate implements View.OnClickListener
{
  private final Fragment mFragment;
  @NonNull
  private final Application mApplication;

  protected abstract void loginOsm();

  public OsmAuthFragmentDelegate(Fragment fragment)
  {
    mFragment = fragment;
    mApplication = fragment.getActivity().getApplication();
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
        Statistics.from(getApplication()).trackAuthRequest(OsmOAuth.AuthType.OSM);
        loginOsm();
        break;
      case R.id.register:
        Statistics.from(getApplication()).trackEvent(Statistics.EventName.EDITOR_REG_REQUEST);
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

        Statistics.from(getApplication()).trackEvent(Statistics.EventName.EDITOR_AUTH_REQUEST_RESULT,
                                                     Statistics.params().add(Statistics.EventParam.IS_SUCCESS, false).add(Statistics.EventParam.TYPE, type.name));
      }
      return;
    }

    OsmOAuth.setAuthorization(auth[0], auth[1], username);
    if (mFragment.isAdded())
      Utils.navigateToParent(mFragment.getActivity());
    Statistics.from(getApplication()).trackEvent(Statistics.EventName.EDITOR_AUTH_REQUEST_RESULT,
                                                 Statistics.params().add(Statistics.EventParam.IS_SUCCESS, true).add(Statistics.EventParam.TYPE, type.name));
  }

  protected void register()
  {
    mFragment.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.OSM_REGISTER)));
  }

  @NonNull
  public Application getApplication()
  {
    return mApplication;
  }
}
