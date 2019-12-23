package com.mapswithme.util;

import androidx.annotation.NonNull;
import androidx.work.Data;
import androidx.work.OneTimeWorkRequest;
import androidx.work.WorkManager;
import com.google.gson.Gson;

public class HttpBackgroundUploader extends AbstractHttpUploader
{
  public HttpBackgroundUploader(@NonNull HttpPayload payload)
  {
    super(payload);
  }

  public void upload()
  {
    Gson gson = new Gson();
    String json = gson.toJson(getPayload());
    Data.Builder builder = new Data.Builder().putString(FileUploadWorker.PARAMS, json);
    OneTimeWorkRequest request = new OneTimeWorkRequest.Builder(FileUploadWorker.class)
        .setInputData(builder.build()).build();
    WorkManager.getInstance().enqueue(request);
  }
}
