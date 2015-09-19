package com.mapswithme.maps.ads;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v7.app.AlertDialog;
import android.view.LayoutInflater;
import android.view.View;

import com.mapswithme.country.ActiveCountryTree;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.util.statistics.Statistics;

public class UpdateMapsDialogFragment extends BaseMwmDialogFragment
{
  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    final Activity activity = getActivity();
    final AlertDialog.Builder builder = new AlertDialog.Builder(activity);
    final LayoutInflater inflater = activity.getLayoutInflater();

    @SuppressLint("InflateParams") final View root = inflater.inflate(R.layout.dialog_new_style, null);
    return builder.
        setView(root).
        setPositiveButton(R.string.update_maps_alert_button_title, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dialog, int which)
          {
            ActiveCountryTree.updateAll();
            LikesManager.setRatingApplied(UpdateMapsDialogFragment.class, true);
            Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.NEW_STYLE_DIALOG_RATED);
          }
        }).create();
  }

  @Override
  public void onCancel(DialogInterface dialog)
  {
    super.onCancel(dialog);
    Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.NEW_STYLE_DIALOG_CANCELLED);
  }
}
