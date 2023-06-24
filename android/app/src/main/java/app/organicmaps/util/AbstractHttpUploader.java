package app.organicmaps.util;

import androidx.annotation.NonNull;

public abstract class AbstractHttpUploader
{
  @NonNull
  private final HttpPayload mPayload;

  AbstractHttpUploader(@NonNull HttpPayload payload)
  {
    mPayload = payload;
  }

  @NonNull
  public HttpPayload getPayload()
  {
    return mPayload;
  }
}
