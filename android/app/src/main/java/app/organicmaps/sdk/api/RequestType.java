package app.organicmaps.sdk.api;

import androidx.annotation.IntDef;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

@Retention(RetentionPolicy.SOURCE)
@IntDef({RequestType.INCORRECT, RequestType.MAP, RequestType.ROUTE, RequestType.SEARCH, RequestType.CROSSHAIR, RequestType.OAUTH2, RequestType.MENU, RequestType.SETTINGS})
public @interface RequestType
{
  // Represents url_scheme::ParsedMapApi::UrlType from c++ part.
  public static final int INCORRECT = 0;
  public static final int MAP = 1;
  public static final int ROUTE = 2;
  public static final int SEARCH = 3;
  public static final int CROSSHAIR = 4;
  public static final int OAUTH2 = 5;
  public static final int MENU = 6;
  public static final int SETTINGS = 7;
}
