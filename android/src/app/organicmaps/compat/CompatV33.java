package app.organicmaps.compat;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;

import java.io.Serializable;

@TargetApi(33)
public class CompatV33 extends CompatV23 implements Compat
{

  @SuppressLint("WrongConstant")
  @Override
  public ResolveInfo resolveActivity(PackageManager packageManager, Intent intent, Compat.ResolveInfoFlags flags) {
    return packageManager.resolveActivity(
        intent,
        PackageManager.ResolveInfoFlags.of(flags.getValue()));
  }

  @Override
  public <T extends Serializable> T getSerializableExtra(Intent intent, String name, Class<T> className) {
    return intent.getSerializableExtra(name, className);
  }
}
