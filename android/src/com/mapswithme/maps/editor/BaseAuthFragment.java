package com.mapswithme.maps.editor;

import android.support.annotation.Size;
import android.support.v7.app.AlertDialog;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmToolbarFragment;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;

public abstract class BaseAuthFragment extends BaseMwmToolbarFragment
{
  protected void processAuth(@Size(2) String[] auth, OsmOAuth.AuthType type)
  {
    if (auth == null)
    {
      if (isAdded())
      {
        new AlertDialog.Builder(getActivity()).setTitle(R.string.editor_login_error_dialog)
                                              .setPositiveButton(android.R.string.ok, null).show();

        Statistics.INSTANCE.trackEvent(Statistics.EventName.EDITOR_AUTH_REQUEST_RESULT,
                                       Statistics.params().add(Statistics.EventParam.IS_SUCCESS, false).add(Statistics.EventParam.TYPE, type.name));
      }
      return;
    }

    OsmOAuth.setAuthorization(auth[0], auth[1]);
    Utils.navigateToParent(getActivity());
    Statistics.INSTANCE.trackEvent(Statistics.EventName.EDITOR_AUTH_REQUEST_RESULT,
                                   Statistics.params().add(Statistics.EventParam.IS_SUCCESS, true).add(Statistics.EventParam.TYPE, type.name));
  }
}
