package com.mapswithme.maps.editor;

import android.content.Intent;
import android.net.Uri;
import android.net.UrlQuerySanitizer;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.Size;
import android.support.v7.app.AlertDialog;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.InputWebView;
import com.mapswithme.maps.widget.ToolbarController;
import com.mapswithme.util.Constants;
import com.mapswithme.util.concurrency.ThreadPool;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.statistics.Statistics;

public class AuthFragment extends BaseAuthFragment implements View.OnClickListener
{
  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_auth_editor, container, false);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mToolbarController.setTitle(R.string.thank_you);
    view.findViewById(R.id.login_osm).setOnClickListener(this);
    view.findViewById(R.id.login_facebook).setOnClickListener(this);
    view.findViewById(R.id.login_google).setOnClickListener(this);
    view.findViewById(R.id.register).setOnClickListener(this);
  }

  @Override
  protected ToolbarController onCreateToolbarController(@NonNull View root)
  {
    return new ToolbarController(root, getActivity())
    {
      @Override
      public void onUpClick()
      {
        Statistics.INSTANCE.trackEvent(Statistics.EventName.EDITOR_AUTH_DECLINED);
        super.onUpClick();
      }
    };
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
    case R.id.login_google:
      Statistics.INSTANCE.trackAuthRequest(OsmOAuth.AuthType.GOOGLE);
      loginWebview(OsmOAuth.AuthType.GOOGLE);
      break;
    case R.id.lost_password:
      Statistics.INSTANCE.trackEvent(Statistics.EventName.EDITOR_LOST_PASSWORD);
      recoverPassword();
      break;
    case R.id.register:
      Statistics.INSTANCE.trackEvent(Statistics.EventName.EDITOR_REG_REQUEST);
      register();
      break;
    }
  }

  protected void loginOsm()
  {
    getMwmActivity().replaceFragment(OsmAuthFragment.class, null, null);
  }

  protected void loginWebview(final OsmOAuth.AuthType type)
  {
    final WebView webview = new InputWebView(getActivity());
    final AlertDialog dialog = new AlertDialog.Builder(getActivity()).setView(webview).create();

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
            if (isAdded())
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
        UiThread.run(new Runnable() {
          @Override
          public void run()
          {
            processAuth(auth, type);
          }
        });
      }
    });
  }

  protected void recoverPassword()
  {
    startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.OSM_RECOVER_PASSWORD)));
  }

  protected void register()
  {
    startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.OSM_REGISTER)));
  }
}
