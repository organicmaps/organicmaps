package app.organicmaps.sdk.api;

import androidx.annotation.IntDef;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

@Retention(RetentionPolicy.SOURCE)
@IntDef({RequestType.INCORRECT, RequestType.MAP, RequestType.ROUTE, RequestType.SEARCH, RequestType.CROSSHAIR,
         RequestType.OAUTH2, RequestType.MENU, RequestType.SETTINGS})
public @interface RequestType
{
  // Represents url_scheme::ParsedMapApi::UrlType from c++ part.
  int INCORRECT = 0;
  int MAP = 1;
  int ROUTE = 2;
  int SEARCH = 3;
  int CROSSHAIR = 4;
  int OAUTH2 = 5;
  int MENU = 6;
  int SETTINGS = 7;
}
