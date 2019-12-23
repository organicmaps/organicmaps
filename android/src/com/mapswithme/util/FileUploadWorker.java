package com.mapswithme.util;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.work.Worker;
import androidx.work.WorkerParameters;
import com.google.gson.Gson;

import java.io.File;
import java.util.Objects;

public class FileUploadWorker extends Worker
{
  static final String PARAMS = "params";

  public FileUploadWorker(@NonNull Context context, @NonNull WorkerParameters workerParams)
  {
    super(context, workerParams);
  }

  @NonNull
  @Override
  public Result doWork()
  {
    String rawJson = Objects.requireNonNull(getInputData().getString(PARAMS));

    Gson gson = new Gson();
    HttpPayload payload = gson.fromJson(rawJson, HttpPayload.class);
    File file = new File(payload.getFilePath());
    if (!file.exists())
      return Result.success();

    HttpUploader uploader = new HttpUploader(payload);
    HttpUploader.Result result = uploader.upload();

    file.delete();
    return result.getHttpCode() / 100 != 2 ? Result.failure() : Result.success();
  }
}
