package app.organicmaps.wear.protocol;

/**
 * Wire contract shared by the phone bridge and the watch app: Data Layer paths and payload keys,
 * plus a protocol version so the two independently-updatable APKs can detect a mismatch.
 */
public final class WearNavigationData
{
  public static final String PATH_NAVIGATION_STATE = "/organicmaps/navigation/state";
  public static final int VERSION = 1;

  public static final String KEY_VERSION = "version";
  public static final String KEY_MODE = "mode";

  private WearNavigationData() {}
}
