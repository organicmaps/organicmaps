package app.organicmaps.wear.protocol;

/**
 * Navigation state mirrored from the phone to the watch.
 *
 * <p>The object keeps the current mode-only payload behind a state type, allowing the codec schema
 * and publisher contract to evolve without replacing the type passed between them.
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
