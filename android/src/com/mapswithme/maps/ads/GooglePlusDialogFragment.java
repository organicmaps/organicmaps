package com.mapswithme.maps.ads;

import android.app.AlertDialog;
import android.app.Dialog;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.app.DialogFragment;
import android.view.LayoutInflater;
import android.view.View;

import com.google.android.gms.plus.PlusOneButton;
import com.mapswithme.maps.R;
import com.mapswithme.util.Constants;

public class GooglePlusDialogFragment extends DialogFragment
{

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    final AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
    final LayoutInflater inflater = getActivity().getLayoutInflater();

    final View root = inflater.inflate(R.layout.fragment_google_plus_dialog, null);
    builder.setView(root);
    PlusOneButton plusButton = (PlusOneButton) root.findViewById(R.id.btn__gplus);
    plusButton.initialize(Constants.Url.PLAY_MARKET_HTTPS_APP_PREFIX + Constants.Package.MWM_PRO_PACKAGE, 1);

    return builder.create();
  }
}
