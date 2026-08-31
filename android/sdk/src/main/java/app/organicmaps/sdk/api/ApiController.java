package app.organicmaps.sdk.api;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.Size;

public final class ApiController
{
  public static native void nativeClearApiPoints();

  @NonNull
  public static native @RequestType int nativeParseAndSetApiUrl(String url);
  public static native ParsedRoutingData nativeGetParsedRoutingData();
  public static native ParsedSearchRequest nativeGetParsedSearchRequest();
  public static native @Nullable String nativeGetParsedAppName();
  public static native @Nullable String nativeGetParsedOAuth2Code();
  @Nullable
  @Size(2)
  public static native double[] nativeGetParsedCenterLatLon();
  public static native @Nullable String nativeGetParsedBackUrl();

  private ApiController()
  {
    throw new UnsupportedOperationException();
  }
}
