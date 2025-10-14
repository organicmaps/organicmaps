package app.organicmaps.background;

import android.content.Context;
import androidx.annotation.NonNull;
import androidx.work.Constraints;
import androidx.work.ExistingWorkPolicy;
import androidx.work.NetworkType;
import androidx.work.OneTimeWorkRequest;
import androidx.work.WorkManager;
import androidx.work.Worker;
import androidx.work.WorkerParameters;
import app.organicmaps.MwmApplication;
import app.organicmaps.sdk.editor.Editor;
import app.organicmaps.sdk.editor.OsmOAuth;
import app.organicmaps.sdk.util.log.Logger;

public class OsmUploadWork extends Worker
{
  private static final String TAG = OsmUploadWork.class.getSimpleName();
  private final Context mContext;
  private final WorkerParameters mWorkerParameters;

  public OsmUploadWork(@NonNull Context context, @NonNull WorkerParameters workerParams)
  {
    super(context, workerParams);
    this.mContext = context;
    this.mWorkerParameters = workerParams;
  }

  /**
   * Starts this worker to upload map edits to osm servers.
   */
  public static void startActionUploadOsmChanges(@NonNull Context context)
  {
    if (Editor.nativeHasSomethingToUpload() && OsmOAuth.isAuthorized())
    {
      final Constraints c = new Constraints.Builder().setRequiredNetworkType(NetworkType.CONNECTED).build();
      final OneTimeWorkRequest wr = new OneTimeWorkRequest.Builder(OsmUploadWork.class).setConstraints(c).build();
      WorkManager.getInstance(context).beginUniqueWork("UploadOsmChanges", ExistingWorkPolicy.KEEP, wr).enqueue();
    }
  }

  @NonNull
  @Override
  public Result doWork()
  {
    if (!MwmApplication.from(mContext).getOrganicMaps().arePlatformAndCoreInitialized())
    {
      Logger.w(TAG, "Application is not initialized, ignoring " + mWorkerParameters);
      return Result.failure();
    }
    Editor.uploadChanges();
    return Result.success();
  }
}
