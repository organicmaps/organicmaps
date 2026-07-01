package app.organicmaps.sdk.wear.gms;

import android.content.Context;
import androidx.annotation.NonNull;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.sdk.wear.WearNavigationPublisher;
import app.organicmaps.wear.protocol.WearNavigationData;
import app.organicmaps.wear.protocol.WearNavigationState;
import app.organicmaps.wear.protocol.WearNavigationStateCodec;
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
    final PutDataRequest request = PutDataRequest.create(WearNavigationData.PATH_NAVIGATION_STATE);
    request.setData(WearNavigationStateCodec.encode(state));
    request.setUrgent();
    Wearable.getDataClient(mContext).putDataItem(request).addOnFailureListener(
        e -> Logger.w(TAG, "Failed to publish Wear navigation state", e));
  }
}
