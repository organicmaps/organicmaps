package app.organicmaps.wear.protocol;

/**
 * Navigation state mirrored from the phone to the watch.
 *
 * <p>The object intentionally wraps the current mode-only payload so later protocol fields can be
 * added without changing the bridge, publisher, and codec APIs.
 */
public final class WearNavigationState
{
  private final WearNavigationMode mMode;

  // Package-private: construct via the factories below or WearNavigationStateCodec.decode.
  WearNavigationState(WearNavigationMode mode)
  {
    mMode = mode;
  }

  public static WearNavigationState normal()
  {
    return new WearNavigationState(WearNavigationMode.NORMAL);
  }

  public static WearNavigationState navigation()
  {
    return new WearNavigationState(WearNavigationMode.NAVIGATION);
  }

  public WearNavigationMode getMode()
  {
    return mMode;
  }
}
