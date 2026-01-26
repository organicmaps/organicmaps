package app.organicmaps.editor.viewmodel;

import androidx.annotation.NonNull;
import androidx.lifecycle.ViewModel;
import app.organicmaps.editor.LoginStatusListener;
import app.organicmaps.sdk.editor.OsmOAuth;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.concurrency.UiThread;

public class OsmLoginViewModel extends ViewModel
{
  @NonNull
  String username = "";
  @NonNull
  String oauthToken = "";
  @NonNull
  private LoginStatusListener listener = new LoginStatusListener() {
    @Override
    public void onSuccess(String oauthToken, String username)
    {}

    @Override
    public void onError(LoginError error)
    {}
  };

  @NonNull
  private UIState uiState = UIState.INITIAL;

  @NonNull
  public UIState getUiState()
  {
    return uiState;
  }

  @NonNull
  public String getUsername()
  {
    return username;
  }

  @NonNull
  public String getOauthToken()
  {
    return oauthToken;
  }

  public void setListener(@NonNull LoginStatusListener listener)
  {
    this.listener = listener;
  }

  public void resetUIState()
  {
    uiState = UIState.INITIAL;
  }

  public void login(@NonNull String username, @NonNull String password)
  {
    this.username = username;
    uiState = UIState.LOADING;

    ThreadPool.getWorker().execute(() -> {
      try
      {
        final String oauthToken = OsmOAuth.nativeAuthWithPassword(username, password);
        final String username1 = (oauthToken == null) ? null : OsmOAuth.nativeGetOsmUsername(oauthToken);

        UiThread.run(() -> {
          if (oauthToken == null || username1 == null)
          {
            uiState = UIState.ERROR;
            listener.onError(LoginError.LOGIN_ERROR);
          }
          else
          {
            uiState = UIState.SUCCESS;
            this.username = username1;
            this.oauthToken = oauthToken;
            listener.onSuccess(oauthToken, username1);
          }
        });
      }
      catch (Exception e)
      {
        UiThread.run(() -> listener.onError(LoginError.LOGIN_ERROR));
      }
    });
  }
}
