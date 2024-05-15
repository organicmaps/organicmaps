package app.organicmaps.settings;

public class MapLocale
{
  private static native String getDefaultMapLocale();
  
  private static native String getCurrentMapLocale();

  private static native void setCurrentMapLocale(String locale);

  public static String getMapLocale()
  {
    return getCurrentMapLocale();
  }

  public static void setMapLocale(String locale)
  {
    setCurrentMapLocale(locale);
  }

  public static void initializeCurrentMapLocale()
  {
    String locale = getCurrentMapLocale();
    setCurrentMapLocale(locale.isEmpty() ? (String)getDefaultMapLocale() : locale);
  }
}
