package app.organicmaps.wear.protocol.gms;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.wear.protocol.WearNavigationData;
import app.organicmaps.wear.protocol.WearNavigationMode;
import app.organicmaps.wear.protocol.WearNavigationState;
import com.google.android.gms.wearable.DataMap;

/** Encodes and decodes Wear navigation state using the Google Wear Data Layer DataMap format. */
public final class WearNavigationDataMapCodec
{
  public static void encode(@NonNull DataMap dataMap, @NonNull WearNavigationState state)
  {
    dataMap.putInt(WearNavigationData.KEY_VERSION, WearNavigationData.VERSION);
    dataMap.putString(WearNavigationData.KEY_MODE, state.getMode().name());
  }

  /**
   * Returns null when the peer protocol version differs or the payload is malformed, so an
   * incompatible peer degrades to no navigation state.
   */
  @Nullable
  public static WearNavigationState decode(@NonNull DataMap dataMap)
  {
    if (dataMap.getInt(WearNavigationData.KEY_VERSION, -1) != WearNavigationData.VERSION)
      return null;

    final String modeName = dataMap.getString(WearNavigationData.KEY_MODE);
    if (modeName == null)
      return null;

    try
    {
      return WearNavigationState.of(WearNavigationMode.valueOf(modeName));
    }
    catch (IllegalArgumentException e)
    {
      return null;
    }
  }

  private WearNavigationDataMapCodec() {}
}
