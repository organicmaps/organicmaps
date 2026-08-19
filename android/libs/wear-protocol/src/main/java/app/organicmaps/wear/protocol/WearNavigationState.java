package app.organicmaps.wear.protocol;

/**
 * Navigation state mirrored from the phone to the watch.
 *
 * <p>The object keeps the current mode-only payload behind a state type, allowing the wire schema
 * and publisher contract to evolve without replacing the type passed between them.
 */
public final class WearNavigationState
{
  private final WearNavigationMode mMode;

  private WearNavigationState(WearNavigationMode mode)
  {
    mMode = mode;
  }

  public static WearNavigationState of(WearNavigationMode mode)
  {
    return new WearNavigationState(mode);
  }

  public static WearNavigationState normal()
  {
    return of(WearNavigationMode.NORMAL);
  }

  public static WearNavigationState navigation()
  {
    return of(WearNavigationMode.NAVIGATION);
  }

  public WearNavigationMode getMode()
  {
    return mMode;
  }
}
