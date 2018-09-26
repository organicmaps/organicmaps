package com.mapswithme.maps.editor;

import android.annotation.SuppressLint;
import android.content.DialogInterface;
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
  private static final String EXTRA_CONGRATS_SHOWN = "CongratsShown";

  private String mViralText;

  private final String viralChangesMsg = MwmApplication.get().getString(R.string.editor_done_dialog_1);
  private final String viralRatingMsg = MwmApplication.get().getString(R.string.editor_done_dialog_2, getUserEditorRank());

  @Nullable
  private Runnable mDismissListener;

  public static boolean shouldDisplay()
  {
    return !MwmApplication.prefs().contains(EXTRA_CONGRATS_SHOWN) &&
           Editor.nativeGetStats()[0] == 2 &&
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
    MwmApplication.prefs().edit().putBoolean(EXTRA_CONGRATS_SHOWN, true).apply();

    @SuppressLint("InflateParams")
    final View root = inflater.inflate(R.layout.fragment_editor_viral, null);
    TextView viralText = (TextView) root.findViewById(R.id.viral);
    initViralText();
    viralText.setText(mViralText);
    root.findViewById(R.id.tell_friend).setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        share();
        dismiss();
        if (mDismissListener != null)
          mDismissListener.run();
        Statistics.INSTANCE.trackEvent(Statistics.EventName.EDITOR_SHARE_CLICK);
      }
    });
    root.findViewById(R.id.close).setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        dismiss();
        if (mDismissListener != null)
          mDismissListener.run();
      }
    });
    Statistics.INSTANCE.trackEvent(Statistics.EventName.EDITOR_SHARE_SHOW,
                                   Statistics.params().add("showed", mViralText.equals(viralChangesMsg) ? "change" : "rating"));
    return root;
  }

  @Override
  public void onCancel(DialogInterface dialog)
  {
    super.onCancel(dialog);
    if (mDismissListener != null)
      mDismissListener.run();
  }

  public void onDismissListener(@Nullable Runnable onDismissListener)
  {
    mDismissListener = onDismissListener;
  }

  private void share()
  {
    SharingHelper.shareViralEditor(getActivity(), R.drawable.img_sharing_editor, R.string.editor_sharing_title, R.string.whatsnew_editor_message_1);
  }

  private void initViralText()
  {
    mViralText = new Random().nextBoolean() ? viralChangesMsg : viralRatingMsg;
  }

  // Counts fake rank in the rating of editors.
  private static int getUserEditorRank()
  {
    return 1000 + new Random().nextInt(1000);
  }
}
