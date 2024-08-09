package app.organicmaps.editor;

import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import app.organicmaps.Framework;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmToolbarFragment;
import app.organicmaps.util.Constants;
import app.organicmaps.util.DateUtils;
import app.organicmaps.util.InputUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.util.concurrency.ThreadPool;
import app.organicmaps.util.concurrency.UiThread;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.google.android.material.textfield.TextInputEditText;

public class OsmLoginFragment extends BaseMwmToolbarFragment
{
  private ProgressBar mProgress;
  private Button mLoginButton;
  private Button mLoginWebsiteButton;
  private Button mLostPasswordButton;
  private TextInputEditText mLoginInput;
  private TextInputEditText mPasswordInput;

  private String mArgOAuth2Code;

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
    getToolbarController().setTitle(R.string.login);
    mLoginInput = view.findViewById(R.id.osm_username);
    mPasswordInput = view.findViewById(R.id.osm_password);
    mLoginWebsiteButton = view.findViewById(R.id.login_website);
    mLoginWebsiteButton.setOnClickListener((v) -> loginWithBrowser());
    mLoginButton = view.findViewById(R.id.login);
    mLoginButton.setOnClickListener((v) -> login());
    mLostPasswordButton = view.findViewById(R.id.lost_password);
    mLostPasswordButton.setOnClickListener((v) -> Utils.openUrl(requireActivity(), Constants.Url.OSM_RECOVER_PASSWORD));
    Button registerButton = view.findViewById(R.id.register);
    registerButton.setOnClickListener((v) -> Utils.openUrl(requireActivity(), Constants.Url.OSM_REGISTER));
    mProgress = view.findViewById(R.id.osm_login_progress);
    final String dataVersion = DateUtils.getShortDateFormatter().format(Framework.getDataVersion());
    ((TextView) view.findViewById(R.id.osm_presentation))
        .setText(getString(R.string.osm_presentation, dataVersion));

    readArguments();
    if (mArgOAuth2Code != null && !mArgOAuth2Code.isEmpty())
      continueOAuth2Flow(mArgOAuth2Code);
  }

  private void readArguments()
  {
    final Bundle arguments = getArguments();
    if (arguments == null)
      return;

    mArgOAuth2Code = arguments.getString(OsmLoginActivity.EXTRA_OAUTH2CODE);
  }

  private void login()
  {
    InputUtils.hideKeyboard(mLoginInput);
    final String username = mLoginInput.getText().toString().trim();
    final String password = mPasswordInput.getText().toString();
    enableInput(false);
    UiUtils.show(mProgress);
    mLoginButton.setText("");

    ThreadPool.getWorker().execute(() -> {
      final String oauthToken = OsmOAuth.nativeAuthWithPassword(username, password);
      final String username1 = (oauthToken == null) ? null : OsmOAuth.nativeGetOsmUsername(oauthToken);
      UiThread.run(() -> processAuth(oauthToken, username1));
    });
  }

  private void loginWithBrowser()
  {
    enableInput(false);

    String[] oauthParams = OsmOAuth.nativeOAuthParams();
    Uri oauth2url = Uri.parse(oauthParams[0] + "/oauth2/authorize")
            .buildUpon()
            .appendQueryParameter("client_id", oauthParams[1])
            .appendQueryParameter("scope", oauthParams[3])
            .appendQueryParameter("redirect_uri", oauthParams[4])
            .appendQueryParameter("response_type", "code")
            .build();
    try {
      Intent myIntent = new Intent(Intent.ACTION_VIEW, oauth2url);
      startActivity(myIntent);
    } catch (ActivityNotFoundException e) {
      //Toast.makeText(this, "No application can handle this request."
      //        + " Please install a webbrowser",  Toast.LENGTH_LONG).show();
      e.printStackTrace();
    }
  }

  private void enableInput(boolean enable)
  {
    mPasswordInput.setEnabled(enable);
    mLoginInput.setEnabled(enable);
    mLoginButton.setEnabled(enable);
    mLostPasswordButton.setEnabled(enable);
    mLoginWebsiteButton.setEnabled(enable);
  }

  // This method is called by MwmActivity & UrlProcessor when "om://oauth2/osm/callback?code=XXX" is handled
  private void continueOAuth2Flow(String oauth2code)
  {
    if (!isAdded())
      return;

    if (oauth2code == null || oauth2code.length()==0)
    {
      enableInput(true);
      onAuthFail();
    }
    else
    {
      // Do not enable UI until we finish interaction with OAuth2 API
      enableInput(false);

      ThreadPool.getWorker().execute(() -> {
        // Finish OAuth2 auth flow and get username for UI.
        final String oauthToken = OsmOAuth.nativeAuthWithOAuth2Code(oauth2code);
        final String username = (oauthToken == null) ? null : OsmOAuth.nativeGetOsmUsername(oauthToken);
        UiThread.run(() -> {
          enableInput(true);
          processAuth(oauthToken, username);
        });
      });
    }
  }

  private void processAuth(String oauthToken, String username)
  {
    if (!isAdded())
      return;

    enableInput(true);
    UiUtils.hide(mProgress);
    mLoginButton.setText(R.string.login_osm);
    if (oauthToken == null)
      onAuthFail();
    else
      onAuthSuccess(oauthToken, username);
  }

  private void onAuthFail()
  {
    new MaterialAlertDialogBuilder(requireActivity(), R.style.MwmTheme_AlertDialog)
        .setTitle(R.string.editor_login_error_dialog)
        .setPositiveButton(R.string.ok, null)
        .show();
  }

  private void onAuthSuccess(String oauthToken, String username)
  {
    OsmOAuth.setAuthorization(requireContext(), oauthToken, username);
    final Bundle extras = requireActivity().getIntent().getExtras();
    if (extras != null && extras.getBoolean(ProfileActivity.EXTRA_REDIRECT_TO_PROFILE, false))
      startActivity(new Intent(requireContext(), ProfileActivity.class));
    requireActivity().finish();
  }
}
