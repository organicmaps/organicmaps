package app.organicmaps.util;

import androidx.annotation.NonNull;
import androidx.work.Constraints;
import androidx.work.Data;
import androidx.work.NetworkType;
import androidx.work.OneTimeWorkRequest;
import androidx.work.WorkManager;
import com.google.gson.Gson;
import app.organicmaps.util.log.Logger;

public class HttpBackgroundUploader extends AbstractHttpUploader
{
  private static final String TAG = HttpBackgroundUploader.class.getSimpleName();

  public HttpBackgroundUploader(@NonNull HttpPayload payload)
  {
    super(payload);
  }

  public void upload()
  {
    Gson gson = new Gson();
    String json = gson.toJson(getPayload());
    Data.Builder builder = new Data.Builder().putString(FileUploadWorker.PARAMS, json);
    Constraints constraints = new Constraints.Builder()
        .setRequiredNetworkType(NetworkType.UNMETERED)
        .build();
    OneTimeWorkRequest request = new OneTimeWorkRequest.Builder(FileUploadWorker.class)
        .setConstraints(constraints)
        .setInputData(builder.build()).build();
    Logger.d(TAG, "Request " + request + "' going to be enqueued");
    WorkManager.getInstance().enqueue(request);
  }
}
