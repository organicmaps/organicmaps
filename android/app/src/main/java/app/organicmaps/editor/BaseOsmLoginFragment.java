package app.organicmaps.editor;

import android.content.Intent;
import android.os.Bundle;
import android.view.Gravity;
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
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmToolbarFragment;
import app.organicmaps.sdk.editor.OsmOAuth;
import app.organicmaps.sdk.util.Constants;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.util.WindowInsetUtils.ScrollableContentInsetsListener;

public abstract class BaseOsmLoginFragment extends BaseMwmToolbarFragment
{
  private ProgressBar mProgress;
  private Button mLoginButton;
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
    mErrorTv = view.findViewById(R.id.tv_error);
    mErrorTv.setGravity(Gravity.CENTER);
    mLoginButton = view.findViewById(R.id.login);
    Button registerButton = view.findViewById(R.id.register);
    registerButton.setOnClickListener((v) -> Utils.openUrl(requireActivity(), Constants.Url.OSM_REGISTER));
    mProgress = view.findViewById(R.id.osm_login_progress);

    setupLogin(view);

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

  protected abstract void setupLogin(@NonNull View view);

  protected void enableInput(boolean enable)
  {
    mLoginButton.setEnabled(enable);
  }

  protected final void onAuthStarted()
  {
    mIsErrorShown = false;
    UiUtils.hide(mErrorTv);
    enableInput(false);
    UiUtils.show(mProgress);
    mLoginButton.setText("");
  }

  protected final void onAuthFail(@NonNull String error)
  {
    if (!isAdded())
      return;

    enableInput(true);
    UiUtils.hide(mProgress);
    mLoginButton.setText(R.string.login_osm);
    mErrorText = error;
    if (mErrorText != null && (mErrorText.contains("NetworkError")))
      mErrorText = getString(R.string.common_check_internet_connection_dialog);

    if (mErrorText != null)
    {
      mErrorTv.setText(mErrorText);
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

  protected final void onAuthSuccess()
  {
    if (!isAdded())
      return;

    enableInput(true);
    UiUtils.hide(mProgress);
    mLoginButton.setText(R.string.login_osm);

    final Bundle extras = requireActivity().getIntent().getExtras();
    if (extras != null && extras.getBoolean("redirectToProfile", false))
      startActivity(new Intent(requireContext(), ProfileActivity.class));
    requireActivity().finish();
  }

  // This method is called by MwmActivity & UrlProcessor when "om://oauth2/osm/callback?code=XXX" is handled
  private void continueOAuth2Flow(@NonNull String oauth2code)
  {
    if (!isAdded())
      return;

    ThreadPool.getWorker().execute(() -> {
      // Finish OAuth2 auth flow and get username for UI.

      final String oauthToken = OsmOAuth.nativeAuthWithOAuth2Code(oauth2code);
      if (oauthToken != null)
      {
        final String username = OsmOAuth.nativeGetOsmUsername(oauthToken);
        OsmOAuth.setAuthorization(oauthToken, username);
      }
      UiThread.run(() -> {
        if (oauthToken != null)
          onAuthSuccess();
        else
          onAuthFail("AuthWithOAuth2Code failed");
      });
    });
  }
}
