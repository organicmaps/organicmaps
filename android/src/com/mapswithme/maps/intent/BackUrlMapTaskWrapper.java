package com.mapswithme.maps.intent;

import android.content.Intent;
import android.net.Uri;
import android.text.TextUtils;

import androidx.annotation.NonNull;
import com.mapswithme.maps.MwmActivity;

public class BackUrlMapTaskWrapper implements MapTask
{
  private static final String BACK_URL_PARAMETER = "back_url";
  private static final long serialVersionUID = 5078514919824594790L;
  @NonNull
  private final MapTask mMapTask;
  @NonNull
  private final String mUrl;

  private BackUrlMapTaskWrapper(@NonNull MapTask mapTask, @NonNull String url)
  {
    mMapTask = mapTask;
    mUrl = url;
  }

  @Override
  public boolean run(@NonNull MwmActivity target)
  {
    final boolean success = mMapTask.run(target);

    final Intent intent = target.getIntent();
    if (intent == null)
      return success;

    Uri uri = Uri.parse(mUrl);
    String backUrl = uri.getQueryParameter(BACK_URL_PARAMETER);
    if (TextUtils.isEmpty(backUrl))
      return success;

    intent.putExtra(MwmActivity.EXTRA_BACK_URL, backUrl);

    return success;
  }

  @NonNull
  static MapTask wrap(@NonNull MapTask task, @NonNull String url)
  {
    return new BackUrlMapTaskWrapper(task, url);
  }
}
