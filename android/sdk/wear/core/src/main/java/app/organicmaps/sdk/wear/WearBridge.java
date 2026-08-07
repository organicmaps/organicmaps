package app.organicmaps.sdk.wear;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.wear.protocol.WearNavigationState;

/**
 * Routes navigation state to a paired Wear OS device.
 *
 * <p>The Google variant bundles {@code :sdk:wear:gms}, whose manifest-merged {@code ContentProvider}
 * registers the real publisher at startup. On F-Droid/Huawei/Web nothing registers and the bridge
 * falls back to a no-op, so callers in common code never reference Play services types.
 */
public final class WearBridge
{
  @NonNull
  private static final WearNavigationPublisher NO_OP = state -> {};

  @Nullable
  private static volatile WearNavigationPublisher sPublisher;

  /** Called once at process startup from the Google Wear module. */
  public static void register(@NonNull WearNavigationPublisher publisher)
  {
    sPublisher = publisher;
  }

  /** Never null: the registered publisher, or a no-op fallback when no Wear bridge is present. */
  @NonNull
  private static WearNavigationPublisher publisher()
  {
    final WearNavigationPublisher publisher = sPublisher;
    return publisher != null ? publisher : NO_OP;
  }

  /** Maps a navigation-active flag to a published state; suitable as a routing state listener. */
  public static void publishNavigating(boolean navigating)
  {
    publisher().publish(navigating ? WearNavigationState.navigation() : WearNavigationState.normal());
  }

  private WearBridge() {}
}
