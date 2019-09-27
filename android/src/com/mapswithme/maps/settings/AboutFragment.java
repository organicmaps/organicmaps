package com.mapswithme.maps.settings;


import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.util.Constants;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.Utils;
import com.mapswithme.util.sharing.ShareOption;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;

public class AboutFragment extends BaseSettingsFragment
                        implements View.OnClickListener
{
  private void setupItem(@IdRes int id, boolean tint, @NonNull View frame)
  {
    TextView view = frame.findViewById(id);
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
    View root = super.onCreateView(inflater, container, savedInstanceState);

    ((TextView) root.findViewById(R.id.version))
        .setText(getString(R.string.version, BuildConfig.VERSION_NAME));

    ((TextView) root.findViewById(R.id.data_version))
        .setText(getString(R.string.data_version, Framework.nativeGetDataVersion()));

    setupItem(R.id.web, true, root);
    setupItem(R.id.facebook, false, root);
    setupItem(R.id.twitter, false, root);
    setupItem(R.id.rate, true, root);
    setupItem(R.id.share, true, root);
    setupItem(R.id.copyright, false, root);
    View termOfUseView = root.findViewById(R.id.term_of_use_link);
    View privacyPolicyView = root.findViewById(R.id.privacy_policy);
    termOfUseView.setOnClickListener(v -> onTermOfUseClick());
    privacyPolicyView.setOnClickListener(v -> onPrivacyPolicyClick());

    return root;
  }

  private void openLink(@NonNull String link)
  {
    Utils.openUrl(getActivity(), link);
  }

  private void onPrivacyPolicyClick()
  {
    openLink(Framework.nativeGetPrivacyPolicyLink());
  }

  private void onTermOfUseClick()
  {
    openLink(Framework.nativeGetTermsOfUseLink());
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

      case R.id.rate:
        Statistics.INSTANCE.trackEvent(Statistics.EventName.Settings.RATE);
        AlohaHelper.logClick(AlohaHelper.Settings.RATE);
        Utils.openAppInMarket(getActivity(), BuildConfig.REVIEW_URL);
        break;

      case R.id.share:
        Statistics.INSTANCE.trackEvent(Statistics.EventName.Settings.TELL_FRIEND);
        AlohaHelper.logClick(AlohaHelper.Settings.TELL_FRIEND);
        ShareOption.AnyShareOption.ANY.share(getActivity(), getString(R.string.tell_friends_text),
                                             R.string.tell_friends);
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
