package com.mapswithme.maps.background;

import android.content.Context;
import android.content.Intent;

import androidx.annotation.NonNull;
import androidx.core.app.JobIntentService;

import com.mapswithme.maps.MwmJobIntentService;
import com.mapswithme.maps.editor.Editor;
import com.mapswithme.maps.scheduling.JobIdMap;

public class OsmUploadService extends MwmJobIntentService
{
  /**
   * Starts this service to upload map edits to osm servers.
   */
  public static void startActionUploadOsmChanges(@NonNull Context context)
  {
    final Intent intent = new Intent(context, OsmUploadService.class);
    JobIntentService.enqueueWork(context.getApplicationContext(), OsmUploadService.class,
                                 JobIdMap.getId(OsmUploadService.class), intent);
  }

  @Override
  protected void onHandleWorkInitialized(@NonNull Intent intent)
  {
    final Context context = getApplicationContext();
    Editor.uploadChanges(context);
  }
}
