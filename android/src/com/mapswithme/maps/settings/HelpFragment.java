package com.mapswithme.maps.settings;

import android.app.AlertDialog;
import android.app.Fragment;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.preference.PreferenceActivity;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.R;
import com.mapswithme.maps.WebContainerDelegate;
import com.mapswithme.maps.widget.BaseShadowController;
import com.mapswithme.maps.widget.ObservableWebView;
import com.mapswithme.maps.widget.WebViewShadowController;
import com.mapswithme.util.Constants;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;

public class HelpFragment extends Fragment
{
  private View mFrame;
  private WebContainerDelegate mDelegate;
  private BaseShadowController mShadowController;

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    mFrame = inflater.inflate(R.layout.fragment_prefs_help, container, false);

    mDelegate = new WebContainerDelegate(mFrame, Constants.Url.FAQ)
    {
      @Override
      protected void doStartActivity(Intent intent)
      {
        startActivity(intent);
      }
    };

    mFrame.findViewById(R.id.feedback).setOnClickListener(new View.OnClickListener()
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
                Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.Settings.FEEDBACK_GENERAL);
                AlohaHelper.logClick(AlohaHelper.Settings.FEEDBACK_GENERAL);

                startActivity(new Intent(Intent.ACTION_SENDTO)
                                  .setData(Utils.buildMailUri(Constants.Email.FEEDBACK, "[" + BuildConfig.VERSION_NAME + "] Feedback", "")));
              }

              private void reportBug()
              {
                Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.Settings.REPORT_BUG);
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

  @Override
  public void onActivityCreated(Bundle savedInstanceState)
  {
    super.onActivityCreated(savedInstanceState);

    if (((PreferenceActivity)getActivity()).onIsMultiPane())
    {
      ((View)mFrame.getParent()).setPadding(0, 0, 0, 0);
      adjustMargins(mDelegate.getWebView());
      mShadowController = new WebViewShadowController((ObservableWebView)mDelegate.getWebView())
                              .addBottomShadow()
                              .attach();
    }
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();
    if (mShadowController != null)
      mShadowController.detach();
  }

  static void adjustMargins(View view)
  {
    int margin = UiUtils.dimen(R.dimen.margin_half);
    ViewGroup.MarginLayoutParams lp = (ViewGroup.MarginLayoutParams) view.getLayoutParams();
    lp.leftMargin = margin;
    lp.rightMargin = margin;
    view.setLayoutParams(lp);
  }
}
