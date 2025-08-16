package app.organicmaps.sdk.sync.engine;

enum FileExistence
{
  Both,
  OnlyLocal,
  OnlyRemote,
  Neither;

  static FileExistence get(boolean localFileExists, boolean remoteFileExists)
  {
    if (localFileExists)
      return remoteFileExists ? Both : OnlyLocal;
    return remoteFileExists ? OnlyRemote : Neither;
  }
}
