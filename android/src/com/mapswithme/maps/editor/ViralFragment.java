package com.mapswithme.maps.editor;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v7.app.AlertDialog;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.TextView;

import java.util.Random;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.util.BottomSheetHelper;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.sharing.ShareOption;
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
        Statistics.INSTANCE.trackEvent(Statistics.EventName.EDITOR_SHARE_CLICK);
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

  private void share()
  {
    // TODO add custom sharing in twitter and url to facebook sharing
    BottomSheetHelper.Builder sheet = BottomSheetHelper.create(getActivity())
                                                       .sheet(R.menu.menu_viral_editor)
                                                       .listener(new MenuItem.OnMenuItemClickListener() {
                                                         @Override
                                                         public boolean onMenuItemClick(MenuItem item)
                                                         {
                                                           if (item.getItemId() == R.id.share_message)
                                                           {
                                                             ShareOption.SMS.share(getActivity(),
                                                                                   getString(R.string.whatsnew_editor_message_1));
                                                           }
                                                           else
                                                           {
                                                             ShareOption.ANY.share(getActivity(),
                                                                                   getString(R.string.whatsnew_editor_message_1),
                                                                                   R.string.editor_sharing_title);
                                                           }

                                                           dismiss();
                                                           return false;
                                                         }
                                                       });

    if (!ShareOption.SMS.isSupported(getActivity()))
      sheet.getMenu().removeItem(R.id.share_message);

    sheet.tint().show();
  }

  @Override
  public void onDismiss(DialogInterface dialog)
  {
    super.onDismiss(dialog);
    // TODO remove before merge
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
