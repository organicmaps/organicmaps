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
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmToolbarFragment;
import app.organicmaps.sdk.util.Constants;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.util.WindowInsetUtils.ScrollableContentInsetsListener;

public abstract class BaseOsmLoginFragment extends BaseMwmToolbarFragment
{
  private ProgressBar mProgress;
  private Button mLoginButton;
  private TextView mErrorTv;
  private String mErrorText;
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

    ScrollView scrollView = view.findViewById(R.id.scrollView);
    ViewCompat.setOnApplyWindowInsetsListener(scrollView, new ScrollableContentInsetsListener(scrollView));

    if (savedInstanceState != null)
    {
      final String error = savedInstanceState.getString(STATE_ERROR_TEXT);
      if (error != null)
        onAuthFail(error);
    }

    OsmOAuthViewModel model = getOAuthViewModel();
    model.getState().observe(getViewLifecycleOwner(), state -> {
      if (state == null)
        return;

      switch (state)
      {
      case AUTHORIZING -> onAuthStarted();
      case SUCCEEDED ->
      {
        onAuthSuccess();
        model.consumeResult();
      }
      case FAILED ->
      {
        onAuthFail(getString(R.string.osm_login_error));
        model.consumeResult();
      }
      }
    });

    // The authorization code is single-use, so only the first creation starts its exchange.
    if (savedInstanceState == null)
    {
      final String code = readOAuth2CodeFromArguments();
      if (code != null && !code.isEmpty())
        model.authorize(code);
    }
  }

  @NonNull
  protected OsmOAuthViewModel getOAuthViewModel()
  {
    return new ViewModelProvider(requireActivity()).get(OsmOAuthViewModel.class);
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
    mErrorText = null;
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
    mErrorText = error.contains("NetworkError") ? getString(R.string.common_check_internet_connection_dialog) : error;
    mErrorTv.setText(mErrorText);
    UiUtils.show(mErrorTv);
  }

  @Override
  public void onSaveInstanceState(@NonNull Bundle outState)
  {
    super.onSaveInstanceState(outState);
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
    if (extras != null && extras.getBoolean(ProfileActivity.EXTRA_REDIRECT_TO_PROFILE, false))
      startActivity(new Intent(requireContext(), ProfileActivity.class));
    requireActivity().finish();
  }
}
