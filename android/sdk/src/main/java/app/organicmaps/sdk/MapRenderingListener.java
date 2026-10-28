package app.organicmaps.sdk;

import androidx.annotation.Keep;

/**
 * Main-thread callbacks for the map renderer, installed by {@link MapController}.
 * Surface creation, restoration, and initial rendering completion are distinct events.
 */
public interface MapRenderingListener
{
  /**
   * Called when the native engine is first created for a graphics surface.
   */
  default void onRenderingCreated() {}

  /**
   * Called after the surface size and widgets have been updated, including after surface restoration.
   */
  default void onRenderingRestored() {}

  /**
   * Called from the native engine when rendering initialization finishes.
   * Use this for work that requires an initialized renderer, rather than merely a loaded framework.
   */
  @Keep
  @SuppressWarnings("unused")
  default void onRenderingInitializationFinished()
  {}
}
