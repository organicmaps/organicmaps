package app.organicmaps.wear;

import android.content.Context;
import androidx.annotation.NonNull;
import app.organicmaps.BuildConfig;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.wear.protocol.WearNavigationData;
import app.organicmaps.wear.protocol.WearNavigationState;
import app.organicmaps.wear.protocol.WearNavigationStateCodec;
import java.lang.reflect.Method;

public final class PhoneWearNavigationStatePublisher
{
  private static final String TAG = PhoneWearNavigationStatePublisher.class.getSimpleName();
  private static final String GOOGLE_FLAVOR = "google";
  private static final PublisherDelegate sDelegate = createDelegate();

  public static void publish(@NonNull Context context, @NonNull WearNavigationState state)
  {
    sDelegate.publish(context.getApplicationContext(), state);
  }

  @NonNull
  private static PublisherDelegate createDelegate()
  {
    if (!BuildConfig.FLAVOR.equals(GOOGLE_FLAVOR))
      return new NoopPublisherDelegate();

    try
    {
      return new ReflectiveWearPublisherDelegate();
    }
    catch (ReflectiveOperationException | LinkageError e)
    {
      Logger.w(TAG, "Wear Data Layer is not available", e);
      return new NoopPublisherDelegate();
    }
  }

  private interface PublisherDelegate
  {
    void publish(@NonNull Context context, @NonNull WearNavigationState state);
  }

  private static final class NoopPublisherDelegate implements PublisherDelegate
  {
    @Override
    public void publish(@NonNull Context context, @NonNull WearNavigationState state)
    {}
  }

  private static final class ReflectiveWearPublisherDelegate implements PublisherDelegate
  {
    @NonNull
    private final Method mCreatePutDataRequest;
    @NonNull
    private final Method mSetData;
    @NonNull
    private final Method mSetUrgent;
    @NonNull
    private final Method mGetDataClient;
    @NonNull
    private final Method mPutDataItem;

    ReflectiveWearPublisherDelegate() throws ReflectiveOperationException
    {
      Class<?> putDataRequestClass = Class.forName("com.google.android.gms.wearable.PutDataRequest");
      Class<?> wearableClass = Class.forName("com.google.android.gms.wearable.Wearable");
      Class<?> dataClientClass = Class.forName("com.google.android.gms.wearable.DataClient");

      mCreatePutDataRequest = putDataRequestClass.getMethod("create", String.class);
      mSetData = putDataRequestClass.getMethod("setData", byte[].class);
      mSetUrgent = putDataRequestClass.getMethod("setUrgent");
      mGetDataClient = wearableClass.getMethod("getDataClient", Context.class);
      mPutDataItem = dataClientClass.getMethod("putDataItem", putDataRequestClass);
    }

    @Override
    public void publish(@NonNull Context context, @NonNull WearNavigationState state)
    {
      try
      {
        byte[] payload = WearNavigationStateCodec.encode(state);
        Object request = mCreatePutDataRequest.invoke(null, WearNavigationData.PATH_NAVIGATION_STATE);
        mSetData.invoke(request, (Object) payload);
        mSetUrgent.invoke(request);

        Object dataClient = mGetDataClient.invoke(null, context);
        mPutDataItem.invoke(dataClient, request);
      }
      catch (ReflectiveOperationException | LinkageError e)
      {
        Logger.w(TAG, "Failed to publish Wear navigation state", e);
      }
    }
  }

  private PhoneWearNavigationStatePublisher() {}
}
