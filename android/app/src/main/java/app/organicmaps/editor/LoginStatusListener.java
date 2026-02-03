package app.organicmaps.editor;

import app.organicmaps.editor.viewmodel.LoginError;

public interface LoginStatusListener
{
  void onSuccess(String oauthToken, String username);
  void onError(LoginError error);
}
