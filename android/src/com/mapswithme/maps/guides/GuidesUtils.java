package com.mapswithme.maps.guides;

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
    return intent;
  }
}
