package com.mapswithme.maps.ads;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v7.app.AlertDialog;
import android.view.LayoutInflater;
import android.view.View;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.util.sharing.ShareOption;
import com.mapswithme.util.statistics.Statistics;

public abstract class BasePedestrianDialogFragment extends BaseMwmDialogFragment
{
  abstract View buildView(LayoutInflater inflater);

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    final AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
    final LayoutInflater inflater = getActivity().getLayoutInflater();

    @SuppressLint("InflateParams") final View root = buildView(inflater);
    return builder.
        setView(root).
        setNegativeButton(R.string.dialog_routing_not_now, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dialog, int which)
          {
            LikesManager.setRatingApplied(BasePedestrianDialogFragment.this.getClass(), true);
            Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.FACEBOOK_PEDESTRIAN_CANCEL);
          }
        }).
        setPositiveButton(R.string.share, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dialog, int which)
          {
            LikesManager.setRatingApplied(BasePedestrianDialogFragment.this.getClass(), true);
            ShareOption.PEDESTRIAN.share(getActivity());
            Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.FACEBOOK_PEDESTRIAN_SHARE);
          }
        }).create();
  }

  @Override
  public void onCancel(DialogInterface dialog)
  {
    super.onCancel(dialog);
    Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.FACEBOOK_PEDESTRIAN_CANCEL);
  }
}
