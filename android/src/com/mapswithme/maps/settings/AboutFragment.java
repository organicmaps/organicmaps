package com.mapswithme.maps.settings;


import android.app.Fragment;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.preference.PreferenceActivity;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.BaseShadowController;
import com.mapswithme.maps.widget.ObservableScrollView;
import com.mapswithme.maps.widget.ScrollViewShadowController;
import com.mapswithme.util.Constants;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.sharing.ShareOption;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;

public class AboutFragment extends Fragment
                        implements View.OnClickListener
{
  private View mFrame;
  private BaseShadowController mShadowController;

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    mFrame = inflater.inflate(R.layout.about, container, false);

    ((TextView)mFrame.findViewById(R.id.version))
      .setText(getString(R.string.version, BuildConfig.VERSION_NAME));

    mFrame.findViewById(R.id.web).setOnClickListener(this);
    mFrame.findViewById(R.id.blog).setOnClickListener(this);
    mFrame.findViewById(R.id.facebook).setOnClickListener(this);
    mFrame.findViewById(R.id.twitter).setOnClickListener(this);
    mFrame.findViewById(R.id.subscribe).setOnClickListener(this);
    mFrame.findViewById(R.id.rate).setOnClickListener(this);
    mFrame.findViewById(R.id.share).setOnClickListener(this);
    mFrame.findViewById(R.id.copyright).setOnClickListener(this);

    return mFrame;
  }

  @Override
  public void onActivityCreated(Bundle savedInstanceState)
  {
    super.onActivityCreated(savedInstanceState);

    if (((PreferenceActivity)getActivity()).onIsMultiPane())
    {
      ((View)mFrame.getParent()).setPadding(0, 0, 0, 0);
      mShadowController = new ScrollViewShadowController((ObservableScrollView)mFrame.findViewById(R.id.content_frame)).attach();
    }
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();

    if (mShadowController != null)
      mShadowController.detach();
  }

  @Override
  public void onClick(View v)
  {
    try
    {
      switch (v.getId())
      {
        case R.id.web:
          Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.Settings.WEB_SITE);
          AlohaHelper.logClick(AlohaHelper.Settings.WEB_SITE);

          startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.WEB_SITE)));
          break;

        case R.id.blog:
          Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.Settings.WEB_BLOG);
          AlohaHelper.logClick(AlohaHelper.Settings.WEB_BLOG);

          startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.WEB_BLOG)));
          break;

        case R.id.facebook:
          Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.Settings.FACEBOOK);
          AlohaHelper.logClick(AlohaHelper.Settings.FACEBOOK);

          UiUtils.showFacebookPage(getActivity());
          break;

        case R.id.twitter:
          Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.Settings.TWITTER);
          AlohaHelper.logClick(AlohaHelper.Settings.TWITTER);

          UiUtils.showTwitterPage(getActivity());
          break;

        case R.id.subscribe:
          Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.Settings.SUBSCRIBE);
          AlohaHelper.logClick(AlohaHelper.Settings.MAIL_SUBSCRIBE);

          startActivity(new Intent(Intent.ACTION_SENDTO)
                            .setData(Utils.buildMailUri(Constants.Email.SUBSCRIBE,
                                                        getString(R.string.subscribe_me_subject),
                                                        getString(R.string.subscribe_me_body))));
          break;

        case R.id.rate:
          Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.Settings.RATE);
          AlohaHelper.logClick(AlohaHelper.Settings.RATE);

          UiUtils.openAppInMarket(getActivity(), BuildConfig.REVIEW_URL);
          break;

        case R.id.share:
          Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.Settings.TELL_FRIEND);
          AlohaHelper.logClick(AlohaHelper.Settings.TELL_FRIEND);

          ShareOption.ANY.share(getActivity(), getString(R.string.tell_friends_text), R.string.tell_friends);
          break;

        case R.id.copyright:
          Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.Settings.COPYRIGHT);
          AlohaHelper.logClick(AlohaHelper.Settings.COPYRIGHT);

          ((SettingsActivity)getActivity()).switchToFragment(CopyrightFragment.class, R.string.copyright);
          break;
      }
    } catch (ActivityNotFoundException e)
    {
      AlohaHelper.logException(e);
    }
  }
}
