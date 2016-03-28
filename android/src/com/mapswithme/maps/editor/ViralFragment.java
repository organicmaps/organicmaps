package com.mapswithme.maps.editor;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v7.app.AlertDialog;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;

import java.util.Random;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.sharing.SharingHelper;
import com.mapswithme.util.statistics.Statistics;

public class ViralFragment extends BaseMwmDialogFragment
{
  private static final String EXTRA_CONTRATS_SHOWN = "CongratsShown";
  private TextView mViral;

  private static String sViralText;

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
    Statistics.INSTANCE.trackEvent(Statistics.EventName.EDITOR_SHARE_SHOW);

    final AlertDialog.Builder builder = new AlertDialog.Builder(getActivity()).setCancelable(true);
    final LayoutInflater inflater = LayoutInflater.from(getActivity());
    @SuppressLint("InflateParams")
    final View root = inflater.inflate(R.layout.fragment_editor_viral, null);
    mViral = (TextView) root.findViewById(R.id.viral);
    initViralText();
    mViral.setText(sViralText);
    root.findViewById(R.id.tell_friend).setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        share();
        dismiss();
        Statistics.INSTANCE.trackEvent(Statistics.EventName.EDITOR_SHARE_CLICK);
      }
    });
    root.findViewById(R.id.close).setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        dismiss();
      }
    });
    return builder.setView(root).create();
  }

  private void share()
  {
    SharingHelper.shareViralEditor(getActivity(), R.drawable.img_sharing_editor, R.string.editor_sharing_title, R.string.whatsnew_editor_message_1);
  }

  private void initViralText()
  {
    if (sViralText != null)
      return;

    switch (new Random().nextInt(2))
    {
    case 0:
      sViralText = getString(R.string.editor_done_dialog_1);
      break;
    case 1:
      sViralText = getString(R.string.editor_done_dialog_2, getUserEditorRank());
      break;
    default:
      sViralText = getString(R.string.editor_done_dialog_3);
    }
  }

  // Counts fake editor rank based on number of total edits made by user.
  private int getUserEditorRank()
  {
    return (int) ((1000 + new Random().nextInt(1000)) / Editor.nativeGetStats()[0] / 10);
  }
}
