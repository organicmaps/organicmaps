package app.organicmaps;

import android.content.Context;
import android.content.Intent;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.core.app.JobIntentService;
import app.organicmaps.util.CrashlyticsUtils;
import app.organicmaps.util.log.Logger;

import androidx.work.Worker;
import androidx.work.WorkerParameters;

import java.util.logging.Logger;

public abstract class MwmJobIntentService extends Worker {
    private static final String TAG = MwmJobIntentService.class.getSimpleName();

    @NonNull
    protected String getTag() {
        return getClass().getSimpleName();
    }

    protected abstract void onHandleWorkInitialized(@NonNull Intent intent);

    @NonNull
    @Override
    public Result doWork() {
        Context applicationContext = getApplicationContext();

        try {

            MwmApplication app = MwmApplication.from(this);
            String msg = "onHandleWork: " + intent;
            Logger.i(TAG, msg);
            CrashlyticsUtils.INSTANCE.log(Log.INFO, getTag(), msg);

            onHandleWorkInitialized(intent);

            return Result.success();
        } catch (Throwable throwable) {
            Logger.w(TAG, "Application is not initialized, ignoring " + intent);
            return Result.failure();;
        }

    }
}

