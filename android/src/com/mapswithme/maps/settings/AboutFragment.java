package com.mapswithme.maps.settings;


import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.IdRes;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.BaseShadowController;
import com.mapswithme.maps.widget.ObservableScrollView;
import com.mapswithme.maps.widget.ScrollViewShadowController;
import com.mapswithme.util.Constants;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.Utils;
import com.mapswithme.util.sharing.ShareOption;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;

public class AboutFragment extends BaseSettingsFragment
                        implements View.OnClickListener
{
  private void setupItem(@IdRes int id, boolean tint)
  {
    TextView view = (TextView)mFrame.findViewById(id);
    view.setOnClickListener(this);
    if (tint)
      Graphics.tint(view);
  }

  @Override
  protected int getLayoutRes()
  {
    return R.layout.about;
  }

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    super.onCreateView(inflater, container, savedInstanceState);

    ((TextView) mFrame.findViewById(R.id.version))
        .setText(getString(R.string.version, BuildConfig.VERSION_NAME));

    ((TextView) mFrame.findViewById(R.id.data_version))
        .setText(getString(R.string.data_version, Framework.nativeGetDataVersion()));

    setupItem(R.id.web, true);
    setupItem(R.id.blog, true);
    setupItem(R.id.facebook, false);
    setupItem(R.id.twitter, false);
    setupItem(R.id.subscribe, true);
    setupItem(R.id.rate, true);
    setupItem(R.id.share, true);
    setupItem(R.id.copyright, false);

    return mFrame;
  }

  @Override
  public void onClick(View v)
  {
    try
    {
      switch (v.getId())
      {
      case R.id.web:
        Statistics.INSTANCE.trackEvent(Statistics.EventName.Settings.WEB_SITE);
        AlohaHelper.logClick(AlohaHelper.Settings.WEB_SITE);
        startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.WEB_SITE)));
        break;

      case R.id.blog:
        Statistics.INSTANCE.trackEvent(Statistics.EventName.Settings.WEB_BLOG);
        AlohaHelper.logClick(AlohaHelper.Settings.WEB_BLOG);
        startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.WEB_BLOG)));
        break;

      case R.id.facebook:
        Statistics.INSTANCE.trackEvent(Statistics.EventName.Settings.FACEBOOK);
        AlohaHelper.logClick(AlohaHelper.Settings.FACEBOOK);
        Utils.showFacebookPage(getActivity());
        break;

      case R.id.twitter:
        Statistics.INSTANCE.trackEvent(Statistics.EventName.Settings.TWITTER);
        AlohaHelper.logClick(AlohaHelper.Settings.TWITTER);
        Utils.showTwitterPage(getActivity());
        break;

      case R.id.subscribe:
        Statistics.INSTANCE.trackEvent(Statistics.EventName.Settings.SUBSCRIBE);
        AlohaHelper.logClick(AlohaHelper.Settings.MAIL_SUBSCRIBE);
        startActivity(new Intent(Intent.ACTION_SENDTO)
                          .setData(Utils.buildMailUri(Constants.Email.SUBSCRIBE,
                                                      getString(R.string.subscribe_me_subject),
                                                      getString(R.string.subscribe_me_body))));
        break;

      case R.id.rate:
        Statistics.INSTANCE.trackEvent(Statistics.EventName.Settings.RATE);
        AlohaHelper.logClick(AlohaHelper.Settings.RATE);
        Utils.openAppInMarket(getActivity(), BuildConfig.REVIEW_URL);
        break;

      case R.id.share:
        Statistics.INSTANCE.trackEvent(Statistics.EventName.Settings.TELL_FRIEND);
        AlohaHelper.logClick(AlohaHelper.Settings.TELL_FRIEND);
        ShareOption.ANY.share(getActivity(), getString(R.string.tell_friends_text), R.string.tell_friends);
        break;

      case R.id.copyright:
        Statistics.INSTANCE.trackEvent(Statistics.EventName.Settings.COPYRIGHT);
        AlohaHelper.logClick(AlohaHelper.Settings.COPYRIGHT);
        getSettingsActivity().replaceFragment(CopyrightFragment.class,
                                              getString(R.string.copyright), null);
        break;
      }
    } catch (ActivityNotFoundException e)
    {
      AlohaHelper.logException(e);
    }
  }
}
