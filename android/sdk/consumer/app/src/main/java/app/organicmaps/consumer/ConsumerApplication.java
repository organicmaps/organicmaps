package app.organicmaps.consumer;

import android.app.Application;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import app.organicmaps.sdk.OrganicMaps;
import java.io.IOException;

public final class ConsumerApplication extends Application
{
  private OrganicMaps mSdk;
  private final MutableLiveData<Boolean> mReady = new MutableLiveData<>(false);

  OrganicMaps getSdk()
  {
    return mSdk;
  }

  LiveData<Boolean> getReady()
  {
    return mReady;
  }

  @Override
  public void onCreate()
  {
    super.onCreate();
    mSdk = new OrganicMaps(this, "consumer-check", getPackageName(), 1, "1");
    try
    {
      // SDK initialization completes on the main thread, like the lifecycle observers.
      mSdk.init(() -> mReady.setValue(true));
    }
    catch (IOException e)
    {
      throw new IllegalStateException("SDK initialization failed", e);
    }
  }
}
