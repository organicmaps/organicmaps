package app.organicmaps.api;

import android.content.Intent;

public class ParsedMwmRequest
{

  private static volatile ParsedMwmRequest sCurrentRequest;

  // pick point mode
  private boolean mPickPoint;

  public boolean isPickPointMode() { return mPickPoint; }

  public static ParsedMwmRequest getCurrentRequest() { return sCurrentRequest; }

  public static void setCurrentRequest(ParsedMwmRequest request) { sCurrentRequest = request; }

  /**
   * Build request from intent extras.
   */
  public static ParsedMwmRequest extractFromIntent(Intent data)
  {
    final ParsedMwmRequest request = new ParsedMwmRequest();

    request.mPickPoint = data.getBooleanExtra(Const.EXTRA_PICK_POINT, false);

    return request;
  }

  /**
   * Do not use constructor externally. Use {@link ParsedMwmRequest#extractFromIntent(android.content.Intent)} instead.
   */
  private ParsedMwmRequest() {}
}
