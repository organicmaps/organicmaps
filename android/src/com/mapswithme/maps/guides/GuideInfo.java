package com.mapswithme.maps.guides;

public class GuideInfo
{
  public final String mAppId;
  public final String mAppUrl;
  public final String mTitle;
  public final String mMessage;
  public final String mName;

  public GuideInfo(String appId, String appUrl, String title, String message, String name)
  {
    this.mAppId = appId;
    this.mAppUrl = appUrl;
    this.mTitle = title;
    this.mMessage = message;
    this.mName = name;
  }
}
