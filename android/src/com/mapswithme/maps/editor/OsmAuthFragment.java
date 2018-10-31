package com.mapswithme.maps.editor;

import android.app.Activity;
import android.content.Intent;
import android.graphics.LightingColorFilter;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.base.BaseMwmToolbarFragment;
import com.mapswithme.maps.widget.ToolbarController;
import com.mapswithme.util.Constants;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.concurrency.ThreadPool;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.statistics.Statistics;

public class OsmAuthFragment extends BaseMwmToolbarFragment implements View.OnClickListener
{
  private OsmAuthFragmentDelegate mDelegate;

  private ProgressBar mProgress;
  private TextView mTvLogin;
  private View mTvLostPassword;

  private static class AuthToolbarController extends ToolbarController
  {
    AuthToolbarController(View root, Activity activity)
    {
      super(root, activity);
      getToolbar().setNavigationIcon(Graphics.tint(activity, activity.getResources().getDrawable(R.drawable.ic_cancel)));
      getToolbar().setTitleTextColor(ThemeUtils.getColor(activity, android.R.attr.textColorPrimary));
    }
  }

  private EditText mEtLogin;
  private EditText mEtPassword;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_osm_login, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mDelegate = new OsmAuthFragmentDelegate(this)
    {
      @Override
      protected void loginOsm()
      {
        ((BaseMwmFragmentActivity) getActivity()).replaceFragment(OsmAuthFragment.class, null, null);
      }
    };
    mDelegate.onViewCreated(view, savedInstanceState);
    getToolbarController().setTitle(R.string.login);
    mEtLogin = (EditText) view.findViewById(R.id.osm_username);
    mEtPassword = (EditText) view.findViewById(R.id.osm_password);
    mTvLogin = (TextView) view.findViewById(R.id.login);
    mTvLogin.setOnClickListener(this);
    mTvLostPassword = view.findViewById(R.id.lost_password);
    mTvLostPassword.setOnClickListener(this);
    mProgress = (ProgressBar) view.findViewById(R.id.osm_login_progress);
    mProgress.getIndeterminateDrawable().setColorFilter(new LightingColorFilter(0xFF000000, 0xFFFFFF));
    UiUtils.hide(mProgress);
  }

  @NonNull
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
    case R.id.login:
      login();
      break;
    case R.id.lost_password:
      recoverPassword();
      break;
    }
  }

  private void login()
  {
    InputUtils.hideKeyboard(mEtLogin);
    final String username = mEtLogin.getText().toString();
    final String password = mEtPassword.getText().toString();
    enableInput(false);
    UiUtils.show(mProgress);
    mTvLogin.setText("");

    ThreadPool.getWorker().execute(new Runnable()
    {
      @Override
      public void run()
      {
        final String[] auth = OsmOAuth.nativeAuthWithPassword(username, password);
        final String username = auth == null ? null : OsmOAuth.nativeGetOsmUsername(auth[0], auth[1]);
        UiThread.run(new Runnable()
        {
          @Override
          public void run()
          {
            if (!isAdded())
              return;

            enableInput(true);
            UiUtils.hide(mProgress);
            mTvLogin.setText(R.string.login);
            mDelegate.processAuth(auth, OsmOAuth.AuthType.OSM, username);
          }
        });
      }
    });
  }

  private void enableInput(boolean enable)
  {
    mEtPassword.setEnabled(enable);
    mEtLogin.setEnabled(enable);
    mTvLogin.setEnabled(enable);
    mTvLostPassword.setEnabled(enable);
  }

  private void recoverPassword()
  {
    Statistics.INSTANCE.trackEvent(Statistics.EventName.EDITOR_LOST_PASSWORD);
    startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(Constants.Url.OSM_RECOVER_PASSWORD)));
  }
}
