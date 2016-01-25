package com.mapswithme.maps.editor;

import android.app.Activity;
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
import android.widget.EditText;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmToolbarFragment;
import com.mapswithme.maps.widget.ToolbarController;
import com.mapswithme.util.Constants;
import com.mapswithme.util.Graphics;
import com.mapswithme.maps.widget.InputWebView;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.concurrency.ThreadPool;
import com.mapswithme.util.concurrency.UiThread;

public class AuthFragment extends BaseMwmToolbarFragment implements View.OnClickListener
{
  protected static class AuthToolbarController extends ToolbarController
  {
    public AuthToolbarController(View root, Activity activity)
    {
      super(root, activity);
      mToolbar.setNavigationIcon(Graphics.tint(activity, ThemeUtils.getResource(activity, R.attr.homeAsUpIndicator)));
    }

    @Override
    public void onUpClick()
    {
      super.onUpClick();
    }
  }

  private EditText mEtLogin;
  private EditText mEtPassword;

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
    mToolbarController.setTitle("Log In");
    view.findViewById(R.id.login_osm).setOnClickListener(this);
    mEtLogin = (EditText) view.findViewById(R.id.osm_username);
    mEtPassword = (EditText) view.findViewById(R.id.osm_password);
    view.findViewById(R.id.login_facebook).setOnClickListener(this);
    view.findViewById(R.id.login_google).setOnClickListener(this);
    view.findViewById(R.id.lost_password).setOnClickListener(this);
    view.findViewById(R.id.register).setOnClickListener(this);
  }

  @Override
  protected ToolbarController onCreateToolbarController(@NonNull View root)
  {
    return new AuthToolbarController(root, getActivity());
  }

  @Override
  public void onClick(View v)
  {
    // TODO show/hide spinners
    switch (v.getId())
    {
    case R.id.login_osm:
      loginOsm();
      break;
    case R.id.login_facebook:
      loginWebview(true);
      break;
    case R.id.login_google:
      loginWebview(false);
      break;
    case R.id.lost_password:
      recoverPassword();
      break;
    case R.id.register:
      register();
      break;
    }
  }

  private void loginOsm()
  {
    final String username = mEtLogin.getText().toString();
    final String password = mEtPassword.getText().toString();

    ThreadPool.getWorker().execute(new Runnable()
    {
      @Override
      public void run()
      {
        final String[] auth;
        auth = OsmOAuth.nativeAuthWithPassword(username, password);

        UiThread.run(new Runnable()
        {
          @Override
          public void run()
          {
            if (!isAdded())
              return;

            processAuth(auth);
          }
        });
      }
    });
  }

  private void processAuth(@Size(2) String[] auth)
  {
    if (auth == null)
    {
      if (isAdded())
      {
        new AlertDialog.Builder(getActivity()).setTitle("Auth error!")
                                              .setPositiveButton(android.R.string.ok, null).show();
      }
      return;
    }

    OsmOAuth.setAuthorization(auth[0], auth[1]);
    Utils.navigateToParent(getActivity());
  }

  private void loginWebview(final boolean facebook)
  {
    final WebView webview = new InputWebView(getActivity());
    final AlertDialog dialog = new AlertDialog.Builder(getActivity()).setView(webview).create();

    ThreadPool.getWorker().execute(new Runnable()
    {
      @Override
      public void run()
      {
        final String[] auth = facebook ? OsmOAuth.nativeGetFacebookAuthUrl()
                                       : OsmOAuth.nativeGetGoogleAuthUrl();

        UiThread.run(new Runnable()
        {
          @Override
          public void run()
          {
            if (isAdded())
              loadWebviewAuth(dialog, webview, auth);
          }
        });
      }
    });

    dialog.show();
  }

  private void loadWebviewAuth(final AlertDialog dialog, final WebView webview, @Size(3) final String[] auth)
  {
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
          UrlQuerySanitizer sanitizer = new UrlQuerySanitizer();
          sanitizer.setAllowUnregisteredParamaters(true);
          sanitizer.parseUrl(url);
          final String verifier = sanitizer.getValue(OsmOAuth.URL_PARAM_VERIFIER);
          finishWebviewAuth(auth[1], auth[2], verifier);
          dialog.cancel();
          return true;
        }

        return false;
      }
    });
    webview.loadUrl(authUrl);
  }

  private void finishWebviewAuth(final String token, final String secret, final String verifier)
  {
    ThreadPool.getWorker().execute(new Runnable() {
      @Override
      public void run()
      {
        final String[] auth = OsmOAuth.nativeAuthWithWebviewToken(token, secret, verifier);
        UiThread.run(new Runnable() {
          @Override
          public void run()
          {
            processAuth(auth);
          }
        });
      }
    });
  }

  private void recoverPassword()
  {
    startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.OSM_RECOVER_PASSWORD)));
  }

  private void register()
  {
    startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.OSM_REGISTER)));
  }
}
