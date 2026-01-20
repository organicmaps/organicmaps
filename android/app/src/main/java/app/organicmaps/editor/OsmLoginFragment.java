package app.organicmaps.editor;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import app.organicmaps.BuildConfig;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmToolbarFragment;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.editor.OsmOAuth;
import app.organicmaps.sdk.util.Constants;
import app.organicmaps.sdk.util.DateUtils;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.util.InputUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.util.WindowInsetUtils.ScrollableContentInsetsListener;
import com.google.android.material.textfield.TextInputEditText;

public class OsmLoginFragment extends BaseMwmToolbarFragment
{
  private ProgressBar mProgress;
  private Button mLoginButton;
  private Button mLostPasswordButton;
  private TextInputEditText mLoginInput;
  private TextInputEditText mPasswordInput;
  private TextView mErrorTv;
  private boolean mIsErrorShown = false;
  private String mErrorText;
  private static final String STATE_ERROR_SHOWN = "state_error_shown";
  private static final String STATE_ERROR_TEXT = "state_error_text";

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
    mErrorTv = view.findViewById(R.id.tv_error);
    mLoginButton = view.findViewById(R.id.login);
    mLostPasswordButton = view.findViewById(R.id.lost_password);
    Button registerButton = view.findViewById(R.id.register);
    registerButton.setOnClickListener((v) -> Utils.openUrl(requireActivity(), Constants.Url.OSM_REGISTER));
    mProgress = view.findViewById(R.id.osm_login_progress);
    final String dataVersion = DateUtils.getShortDateFormatter().format(Framework.getDataVersion());

    if (BuildConfig.FLAVOR.equals("google") && Utils.isBrowserAvailable(requireContext()))
    {
      // Hide login and password inputs and Forgot password button
      UiUtils.hide(view.findViewById(R.id.osm_username_container), view.findViewById(R.id.osm_password_container),
                   mLostPasswordButton);

      mLoginButton.setOnClickListener((v) -> loginWithBrowser());
    }
    else
    {
      mLoginButton.setOnClickListener((v) -> login());
      mLostPasswordButton.setOnClickListener(
          (v) -> Utils.openUrl(requireActivity(), Constants.Url.OSM_RECOVER_PASSWORD));
    }

    String code = readOAuth2CodeFromArguments();
    if (code != null && !code.isEmpty())
      continueOAuth2Flow(code);

    ScrollView scrollView = view.findViewById(R.id.scrollView);
    ViewCompat.setOnApplyWindowInsetsListener(scrollView, new ScrollableContentInsetsListener(scrollView));

    if (savedInstanceState != null)
    {
      mErrorText = savedInstanceState.getString(STATE_ERROR_TEXT);
      if (savedInstanceState.getBoolean(STATE_ERROR_SHOWN, false))
        onAuthFail(mErrorText);
    }
  }

  private String readOAuth2CodeFromArguments()
  {
    final Bundle arguments = getArguments();
    if (arguments == null)
      return null;

    return arguments.getString(OsmLoginActivity.EXTRA_OAUTH2CODE);
  }

  private void login()
  {
    mIsErrorShown = false;
    UiUtils.hide(mErrorTv);
    InputUtils.hideKeyboard(mLoginInput);
    final String username = mLoginInput.getText().toString().trim();
    final String password = mPasswordInput.getText().toString();
    enableInput(false);
    UiUtils.show(mProgress);
    mLoginButton.setText("");

    ThreadPool.getWorker().execute(() -> {
      final String[] authResult = OsmOAuth.nativeAuthWithPassword(username, password);
      final String oauthToken = authResult != null ? authResult[0] : null;
      final String error = (authResult != null && authResult.length > 1) ? authResult[1] : null;
      final String username1 = (oauthToken == null) ? null : OsmOAuth.nativeGetOsmUsername(oauthToken);
      UiThread.run(() -> processAuth(oauthToken, username1, error));
    });
  }

  private void loginWithBrowser()
  {
    Utils.openUri(requireContext(), Uri.parse(OsmOAuth.nativeGetOAuth2Url()), R.string.browser_not_available);
  }

  private void enableInput(boolean enable)
  {
    mPasswordInput.setEnabled(enable);
    mLoginInput.setEnabled(enable);
    mLoginButton.setEnabled(enable);
    mLostPasswordButton.setEnabled(enable);
  }

  private void processAuth(String oauthToken, String username, @Nullable String error)
  {
    if (!isAdded())
      return;

    enableInput(true);
    UiUtils.hide(mProgress);
    mLoginButton.setText(R.string.login_osm);
    if (oauthToken == null)
      onAuthFail(error);
    else
      onAuthSuccess(oauthToken, username);
  }

  private void onAuthFail(@Nullable String error)
  {
    mIsErrorShown = true;
    mErrorText = error;
    if (mErrorTv != null)
    {
      if (mErrorText != null)
        mErrorTv.setText(mErrorText);
      else
        mErrorTv.setText(R.string.editor_login_error_dialog);
      UiUtils.show(mErrorTv);
    }
  }

  @Override
  public void onSaveInstanceState(@NonNull Bundle outState)
  {
    super.onSaveInstanceState(outState);
    outState.putBoolean(STATE_ERROR_SHOWN, mIsErrorShown);
    outState.putString(STATE_ERROR_TEXT, mErrorText);
  }

  private void onAuthSuccess(String oauthToken, String username)
  {
    OsmOAuth.setAuthorization(oauthToken, username);
    final Bundle extras = requireActivity().getIntent().getExtras();
    if (extras != null && extras.getBoolean("redirectToProfile", false))
      startActivity(new Intent(requireContext(), ProfileActivity.class));
    requireActivity().finish();
  }

  // This method is called by MwmActivity & UrlProcessor when "om://oauth2/osm/callback?code=XXX" is handled
  private void continueOAuth2Flow(String oauth2code)
  {
    if (!isAdded())
      return;

    if (oauth2code == null || oauth2code.isEmpty())
      onAuthFail(null);
    else
    {
      ThreadPool.getWorker().execute(() -> {
        // Finish OAuth2 auth flow and get username for UI.
        final String oauthToken = OsmOAuth.nativeAuthWithOAuth2Code(oauth2code);
        final String username = (oauthToken == null) ? null : OsmOAuth.nativeGetOsmUsername(oauthToken);
        UiThread.run(() -> { processAuth(oauthToken, username, null); });
      });
    }
  }
}
