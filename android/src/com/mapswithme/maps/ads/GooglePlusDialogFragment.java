package com.mapswithme.maps.ads;

import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v7.app.AlertDialog;
import android.view.LayoutInflater;

import com.google.android.gms.plus.PlusOneButton;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.util.Constants;
import com.mapswithme.util.Counters;
import com.mapswithme.util.statistics.Statistics;

public class GooglePlusDialogFragment extends BaseMwmDialogFragment
{
  @Override
  public void onResume()
  {
    super.onResume();

    final PlusOneButton plusButton = (PlusOneButton) getDialog().findViewById(R.id.btn__gplus);
    if (plusButton == null)
      return;

    plusButton.initialize(Constants.Url.PLAY_MARKET_HTTPS_APP_PREFIX + Constants.Package.MWM_PRO_PACKAGE, new PlusOneButton.OnPlusOneClickListener()
    {
      @Override
      public void onPlusOneClick(Intent intent)
      {
        Counters.setRatingApplied(GooglePlusDialogFragment.class);
        dismiss();
        startActivityForResult(intent, 0);
      }
    });
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    final AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
    final LayoutInflater inflater = getActivity().getLayoutInflater();

    builder.setView(inflater.inflate(R.layout.fragment_google_plus_dialog, null)).
        setNegativeButton(getString(R.string.remind_me_later), new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dialog, int which)
          {
            Statistics.INSTANCE.trackEvent(Statistics.EventName.PLUS_DIALOG_LATER);
          }
        });

    return builder.create();
  }

  @Override
  public void onCancel(DialogInterface dialog)
  {
    super.onCancel(dialog);
    Statistics.INSTANCE.trackEvent(Statistics.EventName.PLUS_DIALOG_LATER);
  }
}
