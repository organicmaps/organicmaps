package app.organicmaps.settings;

public class MapLanguageCode
{
  private static native String getDefaultMapLanguageCode();
  
  private static native String getCurrentMapLanguageCode();

  private static native void setCurrentMapLanguageCode(String locale);

  public static String getMapLanguageCode()
  {
    return getCurrentMapLanguageCode();
  }

  public static void setMapLanguageCode(String locale)
  {
    setCurrentMapLanguageCode(locale);
  }

  public static void initializeCurrentMapLanguageCode()
  {
    String locale = getCurrentMapLanguageCode();
    setCurrentMapLanguageCode(locale.isEmpty() ? (String)getDefaultMapLanguageCode() : locale);
  }
}
