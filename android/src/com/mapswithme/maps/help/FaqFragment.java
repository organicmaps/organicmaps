package com.mapswithme.maps.help;

import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;

import com.mapswithme.maps.R;
import com.mapswithme.maps.WebContainerDelegate;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.util.Constants;
import com.mapswithme.util.Utils;

public class FaqFragment extends BaseMwmFragment
{
  @NonNull
  private final DialogInterface.OnClickListener mDialogClickListener = new DialogInterface.OnClickListener()
  {
    private void sendGeneralFeedback()
    {
      Utils.sendFeedback(requireActivity());
    }

    private void reportBug()
    {
      Utils.sendBugReport(requireActivity(), "");
    }

    @Override
    public void onClick(DialogInterface dialog, int which)
    {
      switch (which)
      {
        case 0:
          sendGeneralFeedback();
          break;

        case 1:
          reportBug();
          break;
      }
    }
  };

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_prefs_faq, container, false);

    new WebContainerDelegate(root, Constants.Url.FAQ)
    {
      @Override
      protected void doStartActivity(Intent intent)
      {
        startActivity(intent);
      }
    };

    TextView feedback = root.findViewById(R.id.feedback);
    feedback.setOnClickListener(v -> new AlertDialog.Builder(requireActivity())
        .setTitle(R.string.feedback)
        .setNegativeButton(R.string.cancel, null)
        .setItems(new CharSequence[] { getString(R.string.feedback_general),
                                       getString(R.string.report_a_bug) },
                  mDialogClickListener).show());

    return root;
  }
}
