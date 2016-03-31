package com.mapswithme.maps.editor;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
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

  private static String sViralText;

  public static boolean shouldDisplay()
  {
    return !MwmApplication.prefs().contains(EXTRA_CONTRATS_SHOWN) &&
           Editor.nativeGetStats()[0] >= 2 &&
           ConnectionState.isConnected();
  }

  @Override
  protected int getStyle()
  {
    return STYLE_NO_TITLE;
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    MwmApplication.prefs().edit().putBoolean(EXTRA_CONTRATS_SHOWN, true).apply();
    Statistics.INSTANCE.trackEvent(Statistics.EventName.EDITOR_SHARE_SHOW);

    @SuppressLint("InflateParams")
    final View root = inflater.inflate(R.layout.fragment_editor_viral, null);
    TextView viralText = (TextView) root.findViewById(R.id.viral);
    initViralText();
    viralText.setText(sViralText);
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
    return root;
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

  // Counts fake rank in the rating of editors.
  private int getUserEditorRank()
  {
    return 1000 + new Random().nextInt(1000);
  }
}
