package app.organicmaps.editor.viewmodel;

import androidx.lifecycle.ViewModel;
import app.organicmaps.editor.LoginStatusListener;
import app.organicmaps.sdk.editor.OsmOAuth;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.sdk.util.concurrency.UiThread;

public class OsmLoginViewModel extends ViewModel
{
  String username;
  String oauthToken;

  LoginStatusListener listener;

  private UIState uiState = UIState.INITIAL;

  public UIState getUiState()
  {
    return uiState;
  }

  public String getUsername()
  {
    return username;
  }

  public String getOauthToken()
  {
    return oauthToken;
  }

  public void setListener(LoginStatusListener listener)
  {
    this.listener = listener;
  }

  public void login(String username, String password)
  {
    this.username = username;
    uiState = UIState.LOADING;

    ThreadPool.getWorker().execute(() -> {
      try
      {
        final String oauthToken = OsmOAuth.nativeAuthWithPassword(username, password);
        final String username1 = (oauthToken == null) ? null : OsmOAuth.nativeGetOsmUsername(oauthToken);

        UiThread.run(() -> {
          if (oauthToken == null)
          {
            uiState = UIState.ERROR;
            listener.onError();
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
        UiThread.run(() -> listener.onError());
      }
    });
  }
}
