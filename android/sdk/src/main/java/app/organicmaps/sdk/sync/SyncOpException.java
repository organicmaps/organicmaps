package app.organicmaps.sdk.sync;

import app.organicmaps.sdk.util.log.Logger;
import org.json.JSONException;
import org.json.JSONObject;

public sealed abstract class SyncOpException extends Exception permits SyncOpException.NetworkException,
                                                     SyncOpException.AuthExpiredException,
                                                     // TODO(savsch) add storage quota exceeded UI and exception type
                                                     SyncOpException.UnexpectedException
{
  private static final String TAG = SyncOpException.class.getSimpleName();
  private static final String JSON_KEY_TYPE = "typ";
  private static final String JSON_KEY_TIMESTAMP = "tim";
  private static final String JSON_KEY_MESSAGE = "msg";

  private final long mTimestamp; // milliseconds

  public long getTimestampMs()
  {
    return mTimestamp;
  }

  protected SyncOpException()
  {
    this(System.currentTimeMillis());
  }

  protected SyncOpException(String message)
  {
    this(message, System.currentTimeMillis());
  }

  protected SyncOpException(long timestamp)
  {
    super();
    mTimestamp = timestamp;
  }

  protected SyncOpException(String message, long timestamp)
  {
    super(truncateIfNeeded(message));
    mTimestamp = timestamp;
  }

  protected abstract int getTypeId();

  /// Serialize to JSON
  public JSONObject toJson() throws JSONException
  {
    JSONObject json = new JSONObject();
    json.put(JSON_KEY_TYPE, getTypeId());
    json.put(JSON_KEY_TIMESTAMP, mTimestamp);
    json.put(JSON_KEY_MESSAGE, getMessage());
    return json;
  }

  public static SyncOpException fromJson(JSONObject json) throws JSONException
  {
    int typeId = json.getInt(JSON_KEY_TYPE);
    long timestamp = json.getLong(JSON_KEY_TIMESTAMP);
    if (typeId == Types.NETWORK.typeId)
      return new NetworkException(timestamp);
    else if (typeId == Types.AUTH_EXPIRED.typeId)
      return new AuthExpiredException(timestamp);
    else if (typeId == Types.UNEXPECTED.typeId)
    {
      String message = json.optString(JSON_KEY_MESSAGE, "");
      return message.isEmpty() ? new UnexpectedException(timestamp) : new UnexpectedException(message, timestamp);
    }
    else
    {
      // Should be impossible
      Logger.e(TAG, "Unexpected serialized exception type: " + typeId);
      return new UnexpectedException();
    }
  }

  public static final class NetworkException extends SyncOpException
  {
    public NetworkException() {}
    private NetworkException(long timestamp)
    {
      super(timestamp);
    }
    @Override
    protected int getTypeId()
    {
      return Types.NETWORK.typeId;
    }
  }

  public static final class AuthExpiredException extends SyncOpException
  {
    public AuthExpiredException() {}
    private AuthExpiredException(long timestamp)
    {
      super(timestamp);
    }
    @Override
    protected int getTypeId()
    {
      return Types.AUTH_EXPIRED.typeId;
    }
  }

  public static final class UnexpectedException extends SyncOpException
  {
    public UnexpectedException(String message)
    {
      super(message);
    }
    public UnexpectedException() {}
    private UnexpectedException(String message, long timestamp)
    {
      super(message, timestamp);
    }
    private UnexpectedException(long timestamp)
    {
      super(timestamp);
    }
    @Override
    protected int getTypeId()
    {
      return Types.UNEXPECTED.typeId;
    }
  }

  private static String truncateIfNeeded(String message)
  {
    if (message == null)
      return null;
    if (message.length() <= 200)
      return message;
    return message.substring(0, 200) + "...";
  }

  private enum Types
  {
    NETWORK(0),
    AUTH_EXPIRED(1),
    UNEXPECTED(1000);
    final int typeId; // persisted to disk, so must be the same across releases
    Types(int typeId)
    {
      this.typeId = typeId;
    }
  }
}
