package app.organicmaps.sync.googledrive;

public record StoredOAuthParams(String state, String codeVerifier)
{
  String serialize()
  {
    return state + ":" + codeVerifier;
  }

  static StoredOAuthParams deserialize(String params)
  {
    String[] parts = params.split(":");
    return new StoredOAuthParams(parts[0], parts[1]);
  }
}
