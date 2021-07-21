package com.mapswithme.maps.settings;


import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
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
import com.mapswithme.util.Constants;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.SharingUtils;
import com.mapswithme.util.Utils;

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
    setupItem(R.id.github, false, root);
    setupItem(R.id.telegram, false, root);
    setupItem(R.id.instagram, false, root);
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
    openLink(getResources().getString(R.string.privacy_policy_url));
  }

  private void onTermOfUseClick()
  {
    openLink(getResources().getString(R.string.terms_of_use_url));
  }

  @Override
  public void onClick(View v)
  {
    try
    {
      switch (v.getId())
      {
      case R.id.web:
        startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.WEB_SITE)));
        break;

      case R.id.github:
        startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.GITHUB)));
        break;

      case R.id.telegram:
        startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.TELEGRAM)));
        break;

      case R.id.instagram:
        startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.INSTAGRAM)));
        break;

      case R.id.facebook:
        Utils.showFacebookPage(getActivity());
        break;

      case R.id.twitter:
        Utils.showTwitterPage(getActivity());
        break;

      case R.id.rate:
        Utils.openAppInMarket(getActivity(), BuildConfig.REVIEW_URL);
        break;

      case R.id.share:
        SharingUtils.shareApplication(getContext());
        break;

      case R.id.copyright:
        getSettingsActivity().replaceFragment(CopyrightFragment.class,
                                              getString(R.string.copyright), null);
        break;
      }
    } catch (ActivityNotFoundException e)
    {
    }
  }
}
