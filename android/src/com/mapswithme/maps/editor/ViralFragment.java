package com.mapswithme.maps.editor;

import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
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

  @Nullable
  private Runnable mDismissListener;

  public static boolean shouldDisplay(@NonNull Context context)
  {
    return !MwmApplication.prefs(context).contains(EXTRA_CONGRATS_SHOWN) &&
           Editor.nativeGetStats()[0] == 2 &&
           ConnectionState.INSTANCE.isConnected();
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
    MwmApplication.prefs(requireContext()).edit().putBoolean(EXTRA_CONGRATS_SHOWN, true).apply();
    return inflater.inflate(R.layout.fragment_editor_viral, null);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    TextView viralTextView = (TextView) view.findViewById(R.id.viral);
    Context context = requireContext();

    String viralChangesMsg = context.getString(R.string.editor_done_dialog_1);
    String viralRatingMsg = context.getString(R.string.editor_done_dialog_2, getUserEditorRank());
    String viralText = new Random().nextBoolean() ? viralChangesMsg : viralRatingMsg;
    viralTextView.setText(viralText);

    view.findViewById(R.id.tell_friend).setOnClickListener(v -> {
      share();
      dismiss();
      if (mDismissListener != null)
        mDismissListener.run();
      Statistics.INSTANCE.trackEvent(Statistics.EventName.EDITOR_SHARE_CLICK);
    });

    view.findViewById(R.id.close).setOnClickListener(v -> {
      dismiss();
      if (mDismissListener != null)
        mDismissListener.run();
    });

    Statistics.INSTANCE.trackEvent(Statistics.EventName.EDITOR_SHARE_SHOW,
                                   Statistics.params()
                                             .add("showed", viralText.equals(viralChangesMsg) ? "change" : "rating"));
  }

  @Override
  public void onCancel(@NonNull DialogInterface dialog)
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

  // Counts fake rank in the rating of editors.
  private static int getUserEditorRank()
  {
    return 1000 + new Random().nextInt(1000);
  }
}
