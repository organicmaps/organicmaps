package com.mapswithme.maps.guides;

public class GuideInfo
{
  public final String mAppId;
  public final String mAppUrl;
  public final String mTitle;
  public final String mMessage;

  public GuideInfo(String appId, String appUrl, String title, String message)
  {
    this.mAppId = appId;
    this.mAppUrl = appUrl;
    this.mTitle = title;
    this.mMessage = message;
  }
}
