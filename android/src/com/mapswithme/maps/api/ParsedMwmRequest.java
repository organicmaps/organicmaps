package com.mapswithme.maps.api;

import android.content.Intent;

public class ParsedMwmRequest
{

  private static volatile ParsedMwmRequest sCurrentRequest;

  // title
  private String mTitle;
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

    request.mTitle = data.getStringExtra(Const.EXTRA_TITLE);
    request.mPickPoint = data.getBooleanExtra(Const.EXTRA_PICK_POINT, false);

    return request;
  }

  public boolean hasTitle() { return mTitle != null; }

  public String getTitle() { return mTitle; }

  /**
   * Do not use constructor externally. Use {@link ParsedMwmRequest#extractFromIntent(android.content.Intent)} instead.
   */
  private ParsedMwmRequest() {}
}
