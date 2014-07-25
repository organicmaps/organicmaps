package com.mapswithme.maps.guides;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.net.Uri;

import com.mapswithme.util.Constants;

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
    } catch (final NameNotFoundException e)
    {
      return false;
    }
  }

  public static Intent getGoogleStoreIntentForPackage(String packageName)
  {
    final Intent intent = new Intent(Intent.ACTION_VIEW);
    intent.setData(Uri.parse(Constants.Url.PLAY_MARKET_APP_PREFIX + packageName));
    intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET | Intent.FLAG_ACTIVITY_NO_HISTORY);
    return intent;
  }

  public static void openOrDownloadGuide(GuideInfo info, Context context)
  {
    final String packName = info.mAppId;
    if (GuidesUtils.isGuideInstalled(packName, context))
    {
      final Intent i = context.getPackageManager().getLaunchIntentForPackage(packName);
      context.startActivity(i);
    }
    else
      context.startActivity(GuidesUtils.getGoogleStoreIntentForPackage(info.mAppId));
  }
}
