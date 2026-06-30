package app.organicmaps.sdk.wear;

import androidx.annotation.NonNull;
import app.organicmaps.wear.protocol.WearNavigationState;

/**
 * Phone-side port for pushing navigation state to a paired Wear OS device.
 *
 * <p>The real implementation ({@code :sdk:wear:gms}) talks to the Google Wear Data Layer and ships
 * only in flavors that bundle Google Play services. It registers itself at startup through
 * {@link WearBridge}; every other flavor keeps the no-op default, so callers in common, all-flavors
 * code never reference Play services types.
 */
public interface WearNavigationPublisher
{
  void publish(@NonNull WearNavigationState state);
}
