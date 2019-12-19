package com.mapswithme.util;

import androidx.annotation.NonNull;

public class HttpBackgroundUploader extends AbstractHttpUploader
{
  public HttpBackgroundUploader(@NonNull HttpPayload payload)
  {
    super(payload);
  }

  public void upload()
  {
    /* WorkManager */
  }
}
