package com.mapswithme.maps.help;

import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.util.Config;
import com.mapswithme.util.Constants;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.Utils;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

public class HelpFragment extends BaseMwmFragment implements View.OnClickListener
{
  private String mDonateUrl;

  private void setupItem(@IdRes int id, boolean tint, @NonNull View frame)
  {
    TextView view = frame.findViewById(id);
    view.setOnClickListener(this);
    if (tint)
      Graphics.tint(view);
  }

  // Converts 220131 to locale-dependent date (e.g. 31 January 2022),
  private String localDate(long v)
  {
    final Locale locale = getResources().getConfiguration().locale;
    final SimpleDateFormat format = new SimpleDateFormat("yyMMdd", locale);
    final String strVersion = String.valueOf(v);
    try {
      final Date date = format.parse(strVersion);
      if (date == null)
        return strVersion;
      return java.text.DateFormat.getDateInstance().format(date);
    } catch (java.text.ParseException e) {
      e.printStackTrace();
      return strVersion;
    }
  }

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    mDonateUrl = Config.getDonateUrl();
    View root = inflater.inflate(R.layout.about, container, false);

    ((TextView) root.findViewById(R.id.version))
        .setText(BuildConfig.VERSION_NAME);

    ((TextView) root.findViewById(R.id.data_version))
        .setText(getString(R.string.data_version, localDate(Framework.nativeGetDataVersion())));

    setupItem(R.id.news, true, root);
    setupItem(R.id.web, true, root);
    setupItem(R.id.email, true, root);
    setupItem(R.id.github, false, root);
    setupItem(R.id.telegram, false, root);
    setupItem(R.id.instagram, false, root);
    setupItem(R.id.facebook, false, root);
    setupItem(R.id.twitter, false, root);
    setupItem(R.id.matrix, true, root);
    setupItem(R.id.openstreetmap, true, root);
    setupItem(R.id.faq, true, root);
    setupItem(R.id.report, true, root);
    if (TextUtils.isEmpty(mDonateUrl))
    {
      TextView donateView = root.findViewById(R.id.donate);
      donateView.setVisibility(View.GONE);
      TextView supportUsView = root.findViewById(R.id.support_us);
      supportUsView.setVisibility(View.GONE);
    }
    else
    {
      setupItem(R.id.donate, true, root);
      setupItem(R.id.support_us, true, root);
    }
    setupItem(R.id.rate, true, root);
    setupItem(R.id.copyright, false, root);
    View termOfUseView = root.findViewById(R.id.term_of_use_link);
    View privacyPolicyView = root.findViewById(R.id.privacy_policy);
    termOfUseView.setOnClickListener(v -> onTermOfUseClick());
    privacyPolicyView.setOnClickListener(v -> onPrivacyPolicyClick());

    return root;
  }

  private void openLink(@NonNull String link)
  {
    Utils.openUrl(requireActivity(), link);
  }

  private void onPrivacyPolicyClick()
  {
    openLink(getResources().getString(R.string.privacy_policy_url));
  }

  private void onTermOfUseClick()
  {
    openLink(getResources().getString(R.string.terms_of_use_url));
  }

  @Override
  public void onClick(View v)
  {
    final int id = v.getId();
    if (id == R.id.web)
      openLink(Constants.Url.WEB_SITE);
    else if (id == R.id.news)
      openLink(Constants.Url.NEWS);
    else if (id == R.id.email)
      Utils.sendTo(requireContext(), BuildConfig.SUPPORT_MAIL, "Organic Maps");
    else if (id == R.id.github)
      openLink(Constants.Url.GITHUB);
    else if (id == R.id.telegram)
      openLink(getString(R.string.telegram_url));
    else if (id == R.id.instagram)
      openLink(getString(R.string.instagram_url));
    else if (id == R.id.facebook)
      Utils.showFacebookPage(requireActivity());
    else if (id == R.id.twitter)
      openLink(Constants.Url.TWITTER);
    else if (id == R.id.matrix)
      openLink(Constants.Url.MATRIX);
    else if (id == R.id.openstreetmap)
      openLink(Constants.Url.OSM_ABOUT);
    else if (id == R.id.faq)
      ((HelpActivity) requireActivity()).stackFragment(FaqFragment.class, getString(R.string.faq), null);
    else if (id == R.id.report)
      Utils.sendBugReport(requireActivity(), "");
    else if (id == R.id.support_us)
      openLink(Constants.Url.SUPPORT_US);
    else if (id == R.id.donate)
      openLink(mDonateUrl);
    else if (id == R.id.rate)
      Utils.openAppInMarket(requireActivity(), BuildConfig.REVIEW_URL);
    else if (id == R.id.copyright)
      ((HelpActivity) requireActivity()).stackFragment(CopyrightFragment.class, getString(R.string.copyright), null);
  }
}
