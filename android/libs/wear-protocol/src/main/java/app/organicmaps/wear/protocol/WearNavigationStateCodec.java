package app.organicmaps.wear.protocol;

import java.nio.charset.StandardCharsets;
import org.json.JSONException;
import org.json.JSONObject;

public final class WearNavigationStateCodec
{
  public static byte[] encode(WearNavigationState state)
  {
    JSONObject json = new JSONObject();
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
      JSONObject json = new JSONObject(new String(payload, StandardCharsets.UTF_8));
      if (json.optInt(WearNavigationData.KEY_VERSION) != WearNavigationData.VERSION)
        return null;

      WearNavigationMode mode = WearNavigationMode.valueOf(json.getString(WearNavigationData.KEY_MODE));
      return new WearNavigationState(mode);
    }
    catch (IllegalArgumentException | JSONException e)
    {
      return null;
    }
  }

  private WearNavigationStateCodec() {}
}
