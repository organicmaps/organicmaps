package com.mapswithme.maps.editor;

import android.app.Activity;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.app.AlertDialog;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmToolbarFragment;
import com.mapswithme.maps.widget.ToolbarController;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.ThemeUtils;
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

  private static final String PREF_OSM_CONTRIBUTION_CONFIRMED = "OsmContributionConfirmed";

  private View mTermsBlock;
  private View mLoginBlock;
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
    mLoginBlock = view.findViewById(R.id.block_login);
    mTermsBlock = view.findViewById(R.id.block_terms);
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
    switch (v.getId())
    {
    case R.id.login_osm:
      loginOsm();
      break;
    case R.id.login_facebook:
      loginFacebook();
      break;
    case R.id.login_google:
      loginGoogle();
      break;
    case R.id.lost_password:
      recoverPassword();
      break;
    case R.id.register:
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

            if (auth == null)
            {
              new AlertDialog.Builder(getActivity()).setTitle("Auth error!").show();
              return;
            }

            OsmOAuth.setAuthorization(auth[0], auth[1]);
          }
        });
      }
    });
  }

  private void loginFacebook()
  {

  }

  private void loginGoogle()
  {

  }

  private void recoverPassword()
  {

  }


  private void checkContributionTermsConfirmed()
  {
    // TODO
  }
}
