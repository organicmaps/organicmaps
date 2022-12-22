package app.organicmaps;

public interface MapRenderingListener
{
  default void onRenderingCreated() {}

  default void onRenderingRestored() {}

  default void onRenderingInitializationFinished() {}
}
