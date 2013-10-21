package com.mapswithme.maps.guides;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.net.Uri;

public class GuidesUtils
{
  public static boolean isGuideInstalled(String appId, Context context)
  {
    final PackageManager pm = context.getPackageManager();
    try
    {
      // throws if has no package
      pm.getPackageInfo(appId, PackageManager.GET_META_DATA);
      return true;
    }
    catch (final NameNotFoundException e)
    {
      return false;
    }
  }

  public static Intent getGoogleStoreIntentForPackage(String packageName)
  {
    final Intent intent = new Intent(Intent.ACTION_VIEW);
    intent.setData(Uri.parse("market://details?id=" + packageName));
    intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET | Intent.FLAG_ACTIVITY_NO_HISTORY);
    return intent;
  }

  public static void openOrDownloadGuide(GuideInfo info, Activity activity)
  {
    final String packName = info.mAppId;
    if (GuidesUtils.isGuideInstalled(packName, activity))
    {
      final Intent i = activity.getPackageManager().getLaunchIntentForPackage(packName);
      activity.startActivity(i);
    }
    else
      activity.startActivity(GuidesUtils.getGoogleStoreIntentForPackage(info.mAppId));
  }
}
