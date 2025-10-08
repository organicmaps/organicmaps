package app.organicmaps.sdk.sync.engine;

public class LockAlreadyHeldException extends Exception
{
  private final long mRemainingTimeMillis;

  public LockAlreadyHeldException(long remainingTimeMillis)
  {
    mRemainingTimeMillis = remainingTimeMillis;
  }

  public long getExpectedRemainingTimeMs()
  {
    return mRemainingTimeMillis;
  }
}
