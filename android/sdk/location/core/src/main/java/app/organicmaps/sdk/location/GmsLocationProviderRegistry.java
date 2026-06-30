package app.organicmaps.sdk.location;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

/**
 * Process-wide registry for the GMS {@link GmsLocationProviderFactory}.
 *
 * <p>The flavor's GMS location module ({@code :sdk:location:gms:google} for Google/Web/Huawei,
 * {@code :sdk:location:gms:foss} for F-Droid) registers its factory at startup through a
 * manifest-merged {@code ContentProvider}. Until then -- or when no GMS location module is bundled --
 * {@link #factory} returns a stub, so common code never references Play services types directly.
 */
public final class GmsLocationProviderRegistry
{
  @NonNull
  private static final GmsLocationProviderFactory STUB = new GmsLocationProviderFactoryStub();

  @Nullable
  private static volatile GmsLocationProviderFactory sFactory;

  /** Called once at process startup from the flavor's GMS location module. */
  public static void register(@NonNull GmsLocationProviderFactory factory)
  {
    sFactory = factory;
  }

  /** Never null: the registered factory, or a stub fallback when no GMS location module is present. */
  @NonNull
  public static GmsLocationProviderFactory factory()
  {
    final GmsLocationProviderFactory factory = sFactory;
    return factory != null ? factory : STUB;
  }

  private GmsLocationProviderRegistry() {}
}
