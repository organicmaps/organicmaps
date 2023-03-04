package app.organicmaps.background;

import android.content.Context;


import androidx.annotation.NonNull;
import androidx.work.OneTimeWorkRequest;
import androidx.work.WorkManager;
import androidx.work.WorkRequest;
import androidx.work.WorkerParameters;
import app.organicmaps.MwmWork;
import app.organicmaps.editor.Editor;
import app.organicmaps.scheduling.JobIdMap;

public class OsmUploadWork extends MwmWork
{
  public OsmUploadWork(@NonNull Context context, @NonNull WorkerParameters workerParams)
  {
    super(context, workerParams);
  }

  /**
   * Starts this worker to upload map edits to osm servers.
   */
  public static void startActionUploadOsmChanges(@NonNull Context context)
  {
    WorkRequest workReq =new  OneTimeWorkRequest.Builder(OsmUploadWork.class)
        .addTag(JobIdMap.getId(OsmUploadWork.class).toString()).build();
    WorkManager.getInstance(context).enqueue(workReq);
  }


  @Override
  protected void onHandleWorkInitialized(@NonNull WorkerParameters workerParameters)
  {
    final Context context = getApplicationContext();
    Editor.uploadChanges(context);
  }
}
