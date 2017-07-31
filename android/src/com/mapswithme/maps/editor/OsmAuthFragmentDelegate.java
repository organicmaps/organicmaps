package com.mapswithme.maps.editor;

import android.annotation.SuppressLint;
import android.content.Intent;
import android.net.Uri;
import android.net.UrlQuerySanitizer;
import android.os.Bundle;
import android.support.annotation.IdRes;
import android.support.annotation.Nullable;
import android.support.annotation.Size;
import android.support.v4.app.Fragment;
import android.support.v7.app.AlertDialog;
import android.view.View;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.InputWebView;
import com.mapswithme.util.Constants;
import com.mapswithme.util.Utils;
import com.mapswithme.util.concurrency.ThreadPool;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.statistics.Statistics;

public abstract class OsmAuthFragmentDelegate implements View.OnClickListener
{
  private final Fragment mFragment;

  protected abstract void loginOsm();

  public OsmAuthFragmentDelegate(Fragment fragment)
  {
    mFragment = fragment;
  }

  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    for (@IdRes int childId : new int[] {R.id.login_osm, R.id.login_facebook, R.id.register})
    {
      final View v = view.findViewById(childId);
      if (v != null)
        v.setOnClickListener(this);
    }
  }

  @Override
  public void onClick(View v)
  {
    // TODO show/hide spinners
    switch (v.getId())
    {
    case R.id.login_osm:
      Statistics.INSTANCE.trackAuthRequest(OsmOAuth.AuthType.OSM);
      loginOsm();
      break;
    case R.id.login_facebook:
      Statistics.INSTANCE.trackAuthRequest(OsmOAuth.AuthType.FACEBOOK);
      loginWebview(OsmOAuth.AuthType.FACEBOOK);
      break;
    case R.id.register:
      Statistics.INSTANCE.trackEvent(Statistics.EventName.EDITOR_REG_REQUEST);
      register();
      break;
    }
  }

  protected void processAuth(@Size(2) String[] auth, OsmOAuth.AuthType type, String username)
  {
    if (auth == null)
    {
      if (mFragment.isAdded())
      {
        new AlertDialog.Builder(mFragment.getActivity()).setTitle(R.string.editor_login_error_dialog)
                                                        .setPositiveButton(android.R.string.ok, null).show();

        Statistics.INSTANCE.trackEvent(Statistics.EventName.EDITOR_AUTH_REQUEST_RESULT,
                                       Statistics.params().add(Statistics.EventParam.IS_SUCCESS, false).add(Statistics.EventParam.TYPE, type.name));
      }
      return;
    }

    OsmOAuth.setAuthorization(auth[0], auth[1], username);
    if (mFragment.isAdded())
      Utils.navigateToParent(mFragment.getActivity());
    Statistics.INSTANCE.trackEvent(Statistics.EventName.EDITOR_AUTH_REQUEST_RESULT,
                                   Statistics.params().add(Statistics.EventParam.IS_SUCCESS, true).add(Statistics.EventParam.TYPE, type.name));
  }

  @SuppressLint("SetJavaScriptEnabled")
  protected void loginWebview(final OsmOAuth.AuthType type)
  {
    final WebView webview = new InputWebView(mFragment.getActivity());
    webview.getSettings().setJavaScriptEnabled(true);
    final AlertDialog dialog = new AlertDialog.Builder(mFragment.getActivity()).setView(webview).create();

    ThreadPool.getWorker().execute(new Runnable()
    {
      @Override
      public void run()
      {
        final String[] auth = (type == OsmOAuth.AuthType.FACEBOOK) ? OsmOAuth.nativeGetFacebookAuthUrl()
                                                                   : OsmOAuth.nativeGetGoogleAuthUrl();

        UiThread.run(new Runnable()
        {
          @Override
          public void run()
          {
            if (mFragment.isAdded())
              loadWebviewAuth(dialog, webview, auth, type);
          }
        });
      }
    });

    dialog.show();
  }

  protected void loadWebviewAuth(final AlertDialog dialog, final WebView webview, @Size(3) final String[] auth, final OsmOAuth.AuthType type)
  {
    if (auth == null)
    {
      // TODO show some dialog
      return;
    }

    final String authUrl = auth[0];
    webview.setWebViewClient(new WebViewClient()
    {
      @Override
      public boolean shouldOverrideUrlLoading(WebView view, String url)
      {
        if (OsmOAuth.shouldReloadWebviewUrl(url))
        {
          webview.loadUrl(authUrl);
        }
        else if (url.contains(OsmOAuth.URL_PARAM_VERIFIER))
        {
          finishWebviewAuth(auth[1], auth[2], getVerifierFromUrl(url), type);
          dialog.cancel();
          return true;
        }

        return false;
      }

      private String getVerifierFromUrl(String authUrl)
      {
        UrlQuerySanitizer sanitizer = new UrlQuerySanitizer();
        sanitizer.setAllowUnregisteredParamaters(true);
        sanitizer.parseUrl(authUrl);
        return sanitizer.getValue(OsmOAuth.URL_PARAM_VERIFIER);
      }

    });
    webview.loadUrl(authUrl);
  }

  protected void finishWebviewAuth(final String key, final String secret, final String verifier, final OsmOAuth.AuthType type)
  {
    ThreadPool.getWorker().execute(new Runnable() {
      @Override
      public void run()
      {
        final String[] auth = OsmOAuth.nativeAuthWithWebviewToken(key, secret, verifier);
        final String username = auth == null ? null : OsmOAuth.nativeGetOsmUsername(auth[0], auth[1]);
        UiThread.run(new Runnable() {
          @Override
          public void run()
          {
            processAuth(auth, type, username);
          }
        });
      }
    });
  }

  protected void register()
  {
    mFragment.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.OSM_REGISTER)));
  }
}
