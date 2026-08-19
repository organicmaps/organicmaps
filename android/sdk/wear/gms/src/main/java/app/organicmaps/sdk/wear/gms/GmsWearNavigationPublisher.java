package app.organicmaps.sdk.wear.gms;

import android.content.Context;
import android.util.Log;
import androidx.annotation.NonNull;
import app.organicmaps.sdk.wear.WearNavigationPublisher;
import app.organicmaps.wear.protocol.WearNavigationData;
import app.organicmaps.wear.protocol.WearNavigationState;
import app.organicmaps.wear.protocol.gms.WearNavigationDataMapCodec;
import com.google.android.gms.common.api.ApiException;
import com.google.android.gms.common.api.CommonStatusCodes;
import com.google.android.gms.wearable.PutDataMapRequest;
import com.google.android.gms.wearable.PutDataRequest;
import com.google.android.gms.wearable.Wearable;

/**
 * Publishes navigation state to a paired Wear OS device through the Google Wear Data Layer.
 *
 * <p>Lives in the google-only {@code :sdk:wear:gms} module and is registered into
 * {@link app.organicmaps.sdk.wear.WearBridge} at startup by {@link WearBridgeInitProvider}.
 */
final class GmsWearNavigationPublisher implements WearNavigationPublisher
{
  private static final String TAG = GmsWearNavigationPublisher.class.getSimpleName();

  @NonNull
  private final Context mContext;

  GmsWearNavigationPublisher(@NonNull Context context)
  {
    mContext = context.getApplicationContext();
  }

  @Override
  public void publish(@NonNull WearNavigationState state)
  {
    final PutDataMapRequest dataMapRequest = PutDataMapRequest.create(WearNavigationData.PATH_NAVIGATION_STATE);
    WearNavigationDataMapCodec.encode(dataMapRequest.getDataMap(), state);

    final PutDataRequest request = dataMapRequest.asPutDataRequest();
    request.setUrgent();
    Wearable.getDataClient(mContext)
        .putDataItem(request)
        .addOnSuccessListener(item -> Log.d(TAG, "Published Wear navigation state: " + state.getMode()))
        .addOnFailureListener(GmsWearNavigationPublisher::logFailure);
  }

  // API_NOT_CONNECTED reports that the Wearable API is unavailable, not that delivery failed, so it
  // is expected whenever no watch is around and does not deserve a warning.
  private static void logFailure(@NonNull Exception e)
  {
    if (e instanceof ApiException apiError && apiError.getStatusCode() == CommonStatusCodes.API_NOT_CONNECTED)
      Log.d(TAG, "Wear Data Layer unavailable, navigation state not published");
    else
      Log.w(TAG, "Failed to publish Wear navigation state", e);
  }
}
