package app.organicmaps.editor;

import android.view.View;
import android.widget.Button;
import androidx.annotation.NonNull;
import app.organicmaps.R;
import app.organicmaps.sdk.editor.OsmOAuth;
import app.organicmaps.sdk.util.Constants;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.util.InputUtils;
import app.organicmaps.util.Utils;
import com.google.android.material.textfield.TextInputEditText;

public class OsmLoginFragment extends BaseOsmLoginFragment
{
  private TextInputEditText mLoginInput;
  private TextInputEditText mPasswordInput;
  private Button mLostPasswordButton;

  @Override
  protected void setupLogin(@NonNull View view)
  {
    mLoginInput = view.findViewById(R.id.osm_username);
    mPasswordInput = view.findViewById(R.id.osm_password);
    mLostPasswordButton = view.findViewById(R.id.lost_password);
    mLostPasswordButton.setOnClickListener(v -> Utils.openUrl(requireActivity(), Constants.Url.OSM_RECOVER_PASSWORD));
    view.findViewById(R.id.login).setOnClickListener(v -> login());
  }

  @Override
  protected void enableInput(boolean enable)
  {
    super.enableInput(enable);
    mLoginInput.setEnabled(enable);
    mPasswordInput.setEnabled(enable);
    mLostPasswordButton.setEnabled(enable);
  }

  private void login()
  {
    InputUtils.hideKeyboard(mLoginInput);
    final String username = mLoginInput.getText().toString().trim();
    final String password = mPasswordInput.getText().toString();
    onAuthStarted();

    ThreadPool.getWorker().execute(() -> {
      final String[] authResult = new String[1];
      final boolean success = OsmOAuth.nativeAuthWithPassword(username, password, authResult);
      if (success)
      {
        final String oauthToken = authResult[0];
        final String osmUsername = OsmOAuth.nativeGetOsmUsername(oauthToken);
        OsmOAuth.setAuthorization(oauthToken, osmUsername);
        UiThread.run(() -> onAuthSuccess());
      }
      else
      {
        final String error = authResult[0];
        UiThread.run(() -> onAuthFail(error));
      }
    });
  }
}
