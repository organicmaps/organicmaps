package app.organicmaps.editor;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.app.Activity;
import android.app.PendingIntent;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Base64;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;

import androidx.activity.result.ActivityResult;
import androidx.activity.result.ActivityResultCallback;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.IntentSenderRequest;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.Size;

import app.organicmaps.Framework;
import app.organicmaps.MapFragment;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmToolbarFragment;
import app.organicmaps.util.Config;
import app.organicmaps.util.Constants;
import app.organicmaps.util.DateUtils;
import app.organicmaps.util.InputUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.util.concurrency.ThreadPool;
import app.organicmaps.util.concurrency.UiThread;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.google.android.material.textfield.TextInputEditText;
import net.openid.appauth.AppAuthConfiguration;
import net.openid.appauth.AuthState;
import net.openid.appauth.AuthorizationException;
import net.openid.appauth.AuthorizationRequest;
import net.openid.appauth.AuthorizationResponse;
import net.openid.appauth.AuthorizationService;
import net.openid.appauth.AuthorizationServiceConfiguration;
import net.openid.appauth.ClientAuthentication;
import net.openid.appauth.ClientSecretBasic;
import net.openid.appauth.ResponseTypeValues;
import net.openid.appauth.TokenRequest;
import net.openid.appauth.TokenResponse;

import java.security.MessageDigest;
import java.security.SecureRandom;
import app.organicmaps.util.log.Logger;

public class OsmLoginFragment extends BaseMwmToolbarFragment
{
  private static final String TAG = OsmLoginFragment.class.getSimpleName();

  private ProgressBar mProgress;
  private Button mLoginButton;
  private Button mLostPasswordButton;
  private TextInputEditText mLoginInput;
  private TextInputEditText mPasswordInput;

  private AuthorizationService authorizationService;
  private AuthState authState = new AuthState();

  ActivityResultLauncher<IntentSenderRequest> openStreetMapAuthenticationWorkflow = registerForActivityResult(new ActivityResultContracts.StartIntentSenderForResult(), new ActivityResultCallback<ActivityResult>() {
    @Override
    public void onActivityResult(ActivityResult result) {
      if (result.getResultCode() == Activity.RESULT_OK) {
        Logger.d(TAG, String.valueOf(result.getData()));
        AuthorizationResponse authResponse = AuthorizationResponse.fromIntent(result.getData());
        AuthorizationException authException = AuthorizationException.fromIntent(result.getData());
        authState = new AuthState(authResponse, authException);
        if (authException != null) {
          Logger.e(TAG, authException.toJsonString(), authException);
        }
        if (authResponse != null) {
          TokenRequest tokenRequest = authResponse.createTokenExchangeRequest();
          authorizationService.performTokenRequest(tokenRequest, new ClientSecretBasic("gbKl0toQNZAzoeaM1TECP26Q_AG7kx1KUx1pXRNumfY"), new AuthorizationService.TokenResponseCallback() {
            @Override
            public void onTokenRequestCompleted(@Nullable TokenResponse response, @Nullable AuthorizationException ex) {
              if (ex != null) {
                authState = new AuthState();
                Logger.e(TAG, ex.toJsonString(), ex);
              } else {
                if (response != null) {
                  authState.update(response, ex);

                }
              }

              saveOpenStreetMapAuthState();
              //setPreferencesState();
            }
          });
        }

      }

    }
  });

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

    if (!Config.isOsmLoginEnabled(requireContext()))
    {
      new MaterialAlertDialogBuilder(requireActivity(), R.style.MwmTheme_AlertDialog)
          .setMessage(R.string.osm_login_not_available)
          .setCancelable(true)
          .setNegativeButton(R.string.details, (dialog, which) ->
              Utils.openUrl(requireContext(), "https://github.com/organicmaps/organicmaps/issues/7000"))
          .setOnDismissListener(dialog -> requireActivity().finish())
          .show();
    }
  }

  private void login()
  {
    InputUtils.hideKeyboard(mLoginInput);
    //final String username = mLoginInput.getText().toString();
    //final String password = mPasswordInput.getText().toString();
    //enableInput(false);

    UiUtils.show(mProgress);
    mLoginButton.setText("");

    ThreadPool.getWorker().execute(() -> {
      //final String[] auth = OsmOAuth.nativeAuthWithPassword(username, password);
      //final String username1 = auth == null ? null : OsmOAuth.nativeGetOsmUsername(auth[0], auth[1]);
      //UiThread.run(() -> processAuth(auth, username1));

      authorizationService = new AuthorizationService(requireContext(), new AppAuthConfiguration.Builder().build());

      SecureRandom sr = new SecureRandom();
      byte[] ba = new byte[64];
      sr.nextBytes(ba);
      String codeVerifier = android.util.Base64.encodeToString(ba, Base64.URL_SAFE | Base64.NO_PADDING | Base64.NO_WRAP);

      try {
        MessageDigest digest = MessageDigest.getInstance("SHA-256");
        byte[] hash = digest.digest(codeVerifier.getBytes());
        String codeChallenge = android.util.Base64.encodeToString(hash, Base64.URL_SAFE | Base64.NO_PADDING | Base64.NO_WRAP);

        AuthorizationRequest.Builder requestBuilder = new AuthorizationRequest.Builder(
            getAuthorizationServiceConfiguration(),
            getOpenStreetMapClientID(),
            ResponseTypeValues.CODE,
            Uri.parse(getOpenStreetMapRedirect())
        ).setCodeVerifier(codeVerifier, codeChallenge, "S256");

        requestBuilder.setScopes(getOpenStreetMapClientScopes());
        AuthorizationRequest authRequest = requestBuilder.build();
        Intent authIntent = authorizationService.getAuthorizationRequestIntent(authRequest);
        openStreetMapAuthenticationWorkflow.launch(new IntentSenderRequest.Builder(
            PendingIntent.getActivity(getActivity(), 53, authIntent, PendingIntent.FLAG_IMMUTABLE))
                                                       .setFillInIntent(authIntent)
                                                       .build());

      } catch (Exception e) {
        Logger.e(TAG, e.getMessage(), e);
      }
    });
  }

  private void enableInput(boolean enable)
  {
    mPasswordInput.setEnabled(enable);
    mLoginInput.setEnabled(enable);
    mLoginButton.setEnabled(enable);
    mLostPasswordButton.setEnabled(enable);
  }

  private void processAuth(@Size(2) String[] auth, String username)
  {
    if (!isAdded())
      return;

    enableInput(true);
    UiUtils.hide(mProgress);
    mLoginButton.setText(R.string.login_osm);
    if (auth == null)
      onAuthFail();
    else
      onAuthSuccess(auth, username);
  }

  private void onAuthFail()
  {
    new MaterialAlertDialogBuilder(requireActivity(), R.style.MwmTheme_AlertDialog)
        .setTitle(R.string.editor_login_error_dialog)
        .setPositiveButton(R.string.ok, null)
        .show();
  }

  private void onAuthSuccess(@Size(2) String[] auth, String username)
  {
    OsmOAuth.setAuthorization(requireContext(), auth[0], auth[1], username);
    final Bundle extras = requireActivity().getIntent().getExtras();
    if (extras != null && extras.getBoolean("redirectToProfile", false))
      startActivity(new Intent(requireContext(), ProfileActivity.class));
    requireActivity().finish();
  }

  public static AuthorizationServiceConfiguration getAuthorizationServiceConfiguration() {
    return new AuthorizationServiceConfiguration(
        Uri.parse("https://www.openstreetmap.org/oauth2/authorize"),
        Uri.parse("https://www.openstreetmap.org/oauth2/token"),
        null,
        null
    );

  }

  public static String getOpenStreetMapClientID() {
    return "O7RwmIm9_R4NWcTbN-W3m4XE7bRwSusNvIheOn6kMP0";
  }

  public static String getOpenStreetMapRedirect() {
    //Needs to match in androidmanifest.xml
    return "om.oauth2://osm/callback";
  }

  public static String[] getOpenStreetMapClientScopes() {
    return new String[]{"read_prefs", "write_api"};
  }

  void saveOpenStreetMapAuthState() {
    Logger.w(TAG, "Auth state: "+authState.jsonSerializeString());
  }

}
