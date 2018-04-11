package com.mapswithme.maps.auth;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.util.UiUtils;

public class PhoneAuthFragment extends BaseMwmFragment implements OnBackPressListener
{
  private static final String REDIRECT_URL = "https://localhost/";

  @SuppressWarnings("NullableProblems")
  @NonNull
  private WebView mWebView;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mProgress;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_web_view_with_progress, container, false);
  }

  @SuppressLint("SetJavaScriptEnabled")
  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    if (savedInstanceState != null)
      return;

    mWebView = view.findViewById(R.id.webview);
    mProgress = view.findViewById(R.id.progress);
    mWebView.setWebViewClient(new WebViewClient()
    {
      @Override
      public void onPageFinished(WebView view, String url)
      {
        UiUtils.show(mWebView);
        UiUtils.hide(mProgress);
      }

      @Override
      public boolean shouldOverrideUrlLoading(WebView view, String url)
      {
        if (url.contains(REDIRECT_URL + "?code="))
        {
          Intent returnIntent = new Intent();
          returnIntent.putExtra(Constants.EXTRA_PHONE_AUTH_TOKEN,
                                url.substring((REDIRECT_URL + "?code=").length()));

          getActivity().setResult(Activity.RESULT_OK, returnIntent);
          getActivity().finish();

          return true;
        }

        return super.shouldOverrideUrlLoading(view, url);
      }
    });

    mWebView.getSettings().setJavaScriptEnabled(true);
    mWebView.loadUrl(Framework.nativeGetPhoneAuthUrl(REDIRECT_URL));
  }

  @Override
  public boolean onBackPressed()
  {
    if (!mWebView.canGoBack())
      return false;

    mWebView.goBack();
    return true;
  }
}
