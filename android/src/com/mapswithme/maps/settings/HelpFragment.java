package com.mapswithme.maps.settings;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.R;
import com.mapswithme.maps.WebContainerDelegate;
import com.mapswithme.util.Constants;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;

public class HelpFragment extends BaseSettingsFragment
{
  private WebContainerDelegate mDelegate;

  @Override
  protected int getLayoutRes()
  {
    return R.layout.fragment_prefs_help;
  }

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    super.onCreateView(inflater, container, savedInstanceState);

    mDelegate = new WebContainerDelegate(mFrame, Constants.Url.FAQ)
    {
      @Override
      protected void doStartActivity(Intent intent)
      {
        startActivity(intent);
      }
    };

    TextView feedback = (TextView)mFrame.findViewById(R.id.feedback);
    feedback.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        new AlertDialog.Builder(getActivity())
            .setTitle(R.string.feedback)
            .setNegativeButton(R.string.cancel, null)
            .setItems(new CharSequence[] { getString(R.string.feedback_general), getString(R.string.report_a_bug) }, new DialogInterface.OnClickListener()
            {
              private void sendGeneralFeedback()
              {
                Statistics.INSTANCE.trackEvent(Statistics.EventName.Settings.FEEDBACK_GENERAL);
                AlohaHelper.logClick(AlohaHelper.Settings.FEEDBACK_GENERAL);
                startActivity(new Intent(Intent.ACTION_SENDTO)
                                  .setData(Utils.buildMailUri(Constants.Email.FEEDBACK, "[" + BuildConfig.VERSION_NAME + "] Feedback", "")));
              }

              private void reportBug()
              {
                Statistics.INSTANCE.trackEvent(Statistics.EventName.Settings.REPORT_BUG);
                AlohaHelper.logClick(AlohaHelper.Settings.REPORT_BUG);
                Utils.sendSupportMail(getActivity(), "Bugreport from user");
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
            }).show();
      }
    });

    return mFrame;
  }
}
