package app.organicmaps.wear.protocol;

import java.nio.charset.StandardCharsets;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * Encodes/decodes {@link WearNavigationState} for the Wear Data Layer. JSON keeps the payload
 * self-describing and forward-compatible; {@link #decode} rejects a mismatched protocol version so
 * an older peer degrades gracefully instead of misreading the bytes.
 *
 * <p>No nullability annotations here on purpose: wear-protocol is a dependency-free contract module
 * shared by the phone bridge and the watch app. {@link #decode} returns {@code null} on any
 * malformed or version-mismatched payload.
 */
public final class WearNavigationStateCodec
{
  public static byte[] encode(WearNavigationState state)
  {
    final JSONObject json = new JSONObject();
    try
    {
      json.put(WearNavigationData.KEY_VERSION, WearNavigationData.VERSION);
      json.put(WearNavigationData.KEY_MODE, state.getMode().name());
    }
    catch (JSONException e)
    {
      throw new IllegalStateException("Failed to encode Wear navigation state", e);
    }
    return json.toString().getBytes(StandardCharsets.UTF_8);
  }

  public static WearNavigationState decode(byte[] payload)
  {
    if (payload == null || payload.length == 0)
      return null;

    try
    {
      final JSONObject json = new JSONObject(new String(payload, StandardCharsets.UTF_8));
      if (json.optInt(WearNavigationData.KEY_VERSION) != WearNavigationData.VERSION)
        return null;

      final WearNavigationMode mode = WearNavigationMode.valueOf(json.getString(WearNavigationData.KEY_MODE));
      return new WearNavigationState(mode);
    }
    catch (IllegalArgumentException | JSONException e)
    {
      return null;
    }
  }

  private WearNavigationStateCodec() {}
}
