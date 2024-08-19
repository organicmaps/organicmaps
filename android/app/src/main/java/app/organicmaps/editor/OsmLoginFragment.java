package app.organicmaps.editor;

import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import app.organicmaps.Framework;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmToolbarFragment;
import app.organicmaps.util.DateUtils;
import app.organicmaps.util.concurrency.ThreadPool;
import app.organicmaps.util.concurrency.UiThread;

public class OsmLoginFragment extends BaseMwmToolbarFragment
{
  private Button mLoginUsernameButton;
  private Button mLoginWebsiteButton;

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

    mLoginWebsiteButton = view.findViewById(R.id.login_website);
    mLoginWebsiteButton.setOnClickListener((v) -> loginWithBrowser());
    mLoginUsernameButton = view.findViewById(R.id.login_username);
    mLoginUsernameButton.setOnClickListener((v) -> loginUsername());
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

  private void loginUsername()
  {
    OsmLoginBottomFragment bottomSheetFragment = new OsmLoginBottomFragment(this);
    bottomSheetFragment.show(requireActivity().getSupportFragmentManager(), bottomSheetFragment.getTag());
  }

  private void loginWithBrowser()
  {
    try
    {
      Intent myIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(OsmOAuth.nativeGetOAuth2Url()));
      startActivity(myIntent);
    }
    catch (ActivityNotFoundException e)
    {
      //Toast.makeText(this, "No application can handle this request."
      //        + " Please install a webbrowser",  Toast.LENGTH_LONG).show();
      e.printStackTrace();
    }
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
        UiThread.run(() -> {
          processAuth(oauthToken, username);
        });
      });
    }
  }

  public void processAuth(String oauthToken, String username)
  {
    if (!isAdded())
      return;

    if (oauthToken == null)
      onAuthFail();
    else
      onAuthSuccess(oauthToken, username);
  }

  public void onAuthFail()
  {
    new MaterialAlertDialogBuilder(requireActivity(), R.style.MwmTheme_AlertDialog)
            .setTitle(R.string.editor_login_error_dialog)
            .setPositiveButton(R.string.ok, null)
            .show();
  }

  public void onAuthSuccess(String oauthToken, String username)
  {
    OsmOAuth.setAuthorization(requireContext(), oauthToken, username);
    final Bundle extras = requireActivity().getIntent().getExtras();
    if (extras != null && extras.getBoolean(ProfileActivity.EXTRA_REDIRECT_TO_PROFILE, false))
      startActivity(new Intent(requireContext(), ProfileActivity.class));
    requireActivity().finish();
  }
}
