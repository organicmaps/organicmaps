package app.organicmaps.editor;

public interface LoginStatusListener
{
  void onSuccess(String oauthToken, String username);
  void onError();
}
