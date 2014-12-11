package com.mapswithme.maps.Ads;

import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.support.v4.app.DialogFragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebView;
import android.widget.ImageButton;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.R;
import com.mapswithme.util.statistics.Statistics;

import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.util.Locale;

public class BannerDialogFragment extends DialogFragment implements View.OnClickListener
{
  public static final String EXTRA_BANNER = "extra.banner";

  private ImageButton mBtnClose;
  private WebView mWvBanner;
  private Banner mBanner;

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    getActivity().setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);

    mBanner = getArguments().getParcelable(EXTRA_BANNER);

    final View view = inflater.inflate(R.layout.fragment_pro_routing, container, false);
    mBtnClose = (ImageButton) view.findViewById(R.id.btn__close);
    mBtnClose.setOnClickListener(this);
    mWvBanner = (WebView) view.findViewById(R.id.wv__banner);
    mWvBanner.getSettings().setJavaScriptEnabled(true);
    mWvBanner.setInitialScale(100);
    try
    {
      final View root = getActivity().findViewById(android.R.id.content);
      final StringBuilder builder = new StringBuilder(mBanner.getUrl())
          .append("?Width=").append(URLEncoder.encode(String.valueOf(root.getWidth()), "UTF-8"))
          .append("&Height=").append(URLEncoder.encode(String.valueOf(root.getHeight()), "UTF-8"))
          .append("&Lang=").append(URLEncoder.encode(Locale.getDefault().getLanguage(), "UTF-8"))
          .append("&Pro_URL=").append(URLEncoder.encode(BuildConfig.PRO_URL, "UTF-8"));
      mWvBanner.loadUrl(builder.toString());
    } catch (UnsupportedEncodingException e)
    {
      e.printStackTrace();
    }

    Statistics.INSTANCE.trackSimpleNamedEvent(Statistics.EventName.PROMO_BANNER_SHOWN);

    return view;
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.btn__close:
      dismiss();
      break;
    }
  }

  @Override
  public void onStop()
  {
    super.onStop();

    if (getActivity() != null)
      getActivity().setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
  }
}
