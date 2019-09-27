package com.mapswithme.util.sharing;

import android.content.ComponentName;
import android.content.Intent;
import android.graphics.drawable.Drawable;
import androidx.annotation.NonNull;
import android.text.TextUtils;

import com.google.gson.annotations.SerializedName;
import com.mapswithme.util.Gsonable;

public class SharingTarget implements Gsonable, Comparable<SharingTarget>
{
  @SerializedName("package_name")
  public String packageName;
  @SerializedName("usage_count")
  public int usageCount;
  public transient String name;
  public transient String activityName;
  public transient Drawable drawableIcon;

  @SuppressWarnings("UnusedDeclaration")
  public SharingTarget()
  {}

  public SharingTarget(String packageName)
  {
    this.packageName = packageName;
  }

  @Override
  public int compareTo(@NonNull SharingTarget another)
  {
    return (another.usageCount - usageCount);
  }

  public void setupComponentName(Intent intent)
  {
    if (TextUtils.isEmpty(activityName))
      intent.setPackage(packageName);
    else
      intent.setComponent(new ComponentName(packageName, activityName));
  }
}
