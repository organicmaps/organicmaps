package com.mapswithme.maps.guides;

public class GuideInfo
{
  private final String mAppName;
  private final String mAppId;
  private final String mAppUrl;

  public GuideInfo(String appName, String appId, String appUrl)
  {
    this.mAppName = appName;
    this.mAppId = appId;
    this.mAppUrl = appUrl;
  }

  public String getAppName()
  {
    return mAppName;
  }

  public String getAppId()
  {
    return mAppId;
  }

  public String getAppUrl()
  {
    return mAppUrl;
  }
}
