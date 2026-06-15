package app.organicmaps.wear.protocol;

public final class WearNavigationState
{
  private final WearNavigationMode mMode;

  public WearNavigationState(WearNavigationMode mode)
  {
    mMode = mode;
  }

  public static WearNavigationState normal()
  {
    return new WearNavigationState(WearNavigationMode.NORMAL);
  }

  public WearNavigationMode getMode()
  {
    return mMode;
  }
}
