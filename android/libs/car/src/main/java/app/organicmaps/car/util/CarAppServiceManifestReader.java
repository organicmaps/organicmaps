package app.organicmaps.car.util;

import android.annotation.SuppressLint;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import java.util.List;

public class CarAppServiceManifestReader
{
  private static ComponentName mCarAppService;

  public static ComponentName getCarAppServiceClass(@NonNull CarContext context)
  {
    if (mCarAppService != null)
    {
      return mCarAppService;
    }

    mCarAppService = readServiceClassFromManifest(context);
    if (mCarAppService == null)
    {
      throw new IllegalStateException("No CarAppService found");
    }

    return mCarAppService;
  }

  // Querying our own package (intent.setPackage below) does not require a <queries>
  // declaration, and the app manifest declares one anyway, so lint's package-visibility
  // warning is a false positive here.
  @SuppressLint("QueryPermissionsNeeded")
  @Nullable
  private static ComponentName readServiceClassFromManifest(@NonNull CarContext context)
  {
    final Intent intent = new Intent("androidx.car.app.CarAppService");
    intent.addCategory("androidx.car.app.category.NAVIGATION");
    intent.setPackage(context.getPackageName());

    final PackageManager pm = context.getPackageManager();
    final List<ResolveInfo> services = pm.queryIntentServices(intent, PackageManager.GET_META_DATA);

    if (services.isEmpty())
    {
      return null;
    }

    final ResolveInfo resolveInfo = services.get(0);
    final ServiceInfo serviceInfo = resolveInfo.serviceInfo;

    return new ComponentName(serviceInfo.packageName, serviceInfo.name);
  }
}
