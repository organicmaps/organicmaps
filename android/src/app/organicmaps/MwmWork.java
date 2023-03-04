package app.organicmaps;

import android.content.Context;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.work.Worker;
import androidx.work.WorkerParameters;
import app.organicmaps.util.CrashlyticsUtils;
import app.organicmaps.util.log.Logger;

public abstract class MwmWork extends Worker
{

  private final Context mContext;
  private final WorkerParameters mWorkerParameters;
  private static final String TAG = MwmWork.class.getSimpleName();

  protected abstract void onHandleWorkInitialized(@NonNull WorkerParameters workerParameters);


  @NonNull
  protected String getTag()
  {
    return getClass().getSimpleName();
  }

  public MwmWork(@NonNull Context context, @NonNull WorkerParameters workerParams)
  {
    super(context, workerParams);
    this.mContext=context;
    this.mWorkerParameters=workerParams;
  }

  @NonNull
  @Override
  public Result doWork()
  {
    MwmApplication app = MwmApplication.from(mContext);
    String msg = "onHandleWork: " + mWorkerParameters;
    Logger.i(TAG, msg);
    CrashlyticsUtils.INSTANCE.log(Log.INFO, getTag(), msg);
    if (!app.arePlatformAndCoreInitialized())
    {
      Logger.w(TAG, "Application is not initialized, ignoring " + mWorkerParameters);
      return Result.failure();
    }

    onHandleWorkInitialized(mWorkerParameters);
    return Result.success();
  }
}
