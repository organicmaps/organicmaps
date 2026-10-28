package app.organicmaps.sdk;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import app.organicmaps.sdk.widget.placepage.PlacePageData;

/**
 * Callbacks for the process-wide map selection. Register through {@link Framework#nativePlacePageActivationListener}
 * and remove the same listener through {@link Framework#nativeRemovePlacePageActivationListener} when its host ends.
 */
public interface PlacePageActivationListener
{
  /**
   * Called when a map selection is activated or updated.
   * @param data selected place or other place-page data; inspect its type before casting
   */
  @Keep
  @SuppressWarnings("unused")
  void onPlacePageActivated(@NonNull PlacePageData data);

  /**
   * Called when the current selection is cleared; dismiss any associated host UI.
   */
  @Keep
  @SuppressWarnings("unused")
  void onPlacePageDeactivated();

  /**
   * Called when the native map requests a change to full-screen presentation.
   */
  @Keep
  @SuppressWarnings("unused")
  default void onSwitchFullScreenMode()
  {}
}
