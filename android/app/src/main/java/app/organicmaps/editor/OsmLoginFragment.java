package app.organicmaps.editor;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.BuildConfig;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmToolbarFragment;
import app.organicmaps.editor.viewmodel.LoginError;
import app.organicmaps.editor.viewmodel.OsmLoginViewModel;
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
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.google.android.material.textfield.TextInputEditText;

public class OsmLoginFragment extends BaseMwmToolbarFragment implements LoginStatusListener
{
  private ProgressBar mProgress;
  private Button mLoginButton;
  private Button mLostPasswordButton;
  private TextInputEditText mLoginInput;
  private TextInputEditText mPasswordInput;
  private OsmLoginViewModel mViewModel;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    mViewModel = new ViewModelProvider(requireActivity()).get(OsmLoginViewModel.class);
    return inflater.inflate(R.layout.fragment_osm_login, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    getToolbarController().setTitle(R.string.login);
    mLoginInput = view.findViewById(R.id.osm_username);
    mPasswordInput = view.findViewById(R.id.osm_password);
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
    mViewModel.setListener(this);
    showUpdatedUIState();
  }

  private void validateInput()
  {
    if (mLoginInput.getText().toString().isBlank() || mPasswordInput.getText().toString().isBlank())
    {
      mLoginButton.setEnabled(false);
    }
    TextWatcher watcher = new TextWatcher() {
      @Override
      public void afterTextChanged(Editable s)
      {}

      @Override
      public void beforeTextChanged(CharSequence s, int start, int count, int after)
      {}

      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count)
      {
        mLoginButton.setEnabled(
            !(mLoginInput.getText().toString().isBlank() || mPasswordInput.getText().toString().isBlank()));
      }
    };
    mLoginInput.addTextChangedListener(watcher);
    mPasswordInput.addTextChangedListener(watcher);
  }

  private void showUpdatedUIState()
  {
    switch (mViewModel.getUiState())
    {
    case INITIAL -> validateInput();
    case LOADING -> showLoadingState();
    case SUCCESS ->
    {
      hideLoadingState();
      onSuccess(mViewModel.getOauthToken(), mViewModel.getUsername());
    }
    case ERROR ->
    {
      hideLoadingState();
      onAuthFail();
    }
    }
  }

  private String readOAuth2CodeFromArguments()
  {
    final Bundle arguments = getArguments();
    if (arguments == null)
      return null;

    return arguments.getString(OsmLoginActivity.EXTRA_OAUTH2CODE);
  }

  private void showLoadingState()
  {
    InputUtils.hideKeyboard(mLoginInput);
    enableInput(false);
    UiUtils.show(mProgress);
    mLoginButton.setText("");
  }

  private void hideLoadingState()
  {
    enableInput(true);
    UiUtils.hide(mProgress);
    mLoginButton.setText(R.string.login_osm);
  }

  private void login()
  {
    showLoadingState();
    final String username = mLoginInput.getText().toString().trim();
    final String password = mPasswordInput.getText().toString();
    mViewModel.login(username, password);
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
        .setOnDismissListener((dialog -> mViewModel.resetUIState()))
        .show();
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
      onAuthFail();
    else
    {
      ThreadPool.getWorker().execute(() -> {
        // Finish OAuth2 auth flow and get username for UI.
        final String oauthToken = OsmOAuth.nativeAuthWithOAuth2Code(oauth2code);
        final String username = (oauthToken == null) ? null : OsmOAuth.nativeGetOsmUsername(oauthToken);
        UiThread.run(() -> { processAuth(oauthToken, username); });
      });
    }
  }

  @Override
  public void onSuccess(@NonNull String oauthToken, @NonNull String username)
  {
    hideLoadingState();
    onAuthSuccess(oauthToken, username);
  }

  @Override
  public void onError(LoginError error)
  {
    hideLoadingState();
    onAuthFail();
  }
}
