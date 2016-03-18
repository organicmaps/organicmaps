package com.mapswithme.maps.editor;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.StringRes;
import android.support.v7.app.AlertDialog;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;

import java.util.Random;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.util.ConnectionState;

public class ViralFragment extends BaseMwmDialogFragment
{
  private static final String EXTRA_CONTRATS_SHOWN = "CongratsShown";
  private TextView mViral;

  @StringRes
  private static int sViralText;

  public static boolean shouldDisplay()
  {
    return !MwmApplication.prefs().contains(EXTRA_CONTRATS_SHOWN) &&
           Editor.nativeGetStats()[0] >= 2 &&
           ConnectionState.isConnected();
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    MwmApplication.prefs().edit().putBoolean(EXTRA_CONTRATS_SHOWN, true).apply();

    final AlertDialog.Builder builder = new AlertDialog.Builder(getActivity()).setCancelable(true);
    final LayoutInflater inflater = LayoutInflater.from(getActivity());
    @SuppressLint("InflateParams")
    final View root = inflater.inflate(R.layout.fragment_editor_viral, null);
    mViral = (TextView) root.findViewById(R.id.viral);
    initViralText();
    mViral.setText(sViralText);
    // TODO set rank with correct text.
    root.findViewById(R.id.tell_friend).setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        dismiss();
        // TODO send statistics
        // TODO open some share link
      }
    });
    root.findViewById(R.id.close).setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        dismiss();
        // TODO send statistics
      }
    });
    return builder.setView(root).create();
  }

  @Override
  public void onDismiss(DialogInterface dialog)
  {
    super.onDismiss(dialog);
    // TODO remove before merge
  }

  private void initViralText()
  {
    if (sViralText != 0)
      return;

    if (new Random().nextBoolean())
      // FIXME
//        sViralText = R.string.editor_done_dialog_1;
      sViralText = R.string.dialog_routing_change_start;
    else
//        sViralText = R.string.editor_done_dialog_2;
      sViralText = R.string.dialog_routing_change_end;
  }

  // Counts fake editor rank based on number of total edits made by user.
  private int getUserEditorRank()
  {
    return (int) ((1000 + new Random().nextInt(1000)) / Editor.nativeGetStats()[0] / 10);
  }
}
