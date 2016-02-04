package com.mapswithme.maps.editor;

import android.support.annotation.Size;
import android.support.v7.app.AlertDialog;

import com.mapswithme.maps.base.BaseMwmToolbarFragment;
import com.mapswithme.util.Utils;

public abstract class BaseAuthFragment extends BaseMwmToolbarFragment
{
  protected void processAuth(@Size(2) String[] auth)
  {
    if (auth == null)
    {
      if (isAdded())
      {
        new AlertDialog.Builder(getActivity()).setTitle("Auth error!")
                                              .setPositiveButton(android.R.string.ok, null).show();
      }
      return;
    }

    OsmOAuth.setAuthorization(auth[0], auth[1]);
    Utils.navigateToParent(getActivity());
  }
}
