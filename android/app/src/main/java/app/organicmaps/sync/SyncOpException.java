package app.organicmaps.sync;

public sealed abstract class SyncOpException extends Exception permits SyncOpException.NetworkException,
                                                     SyncOpException.AuthExpiredException,
                                                     // TODO(savsch) add storage quota exceeded UI and exception type
                                                     SyncOpException.UnexpectedException
{
  protected SyncOpException()
  {
    super();
  }

  protected SyncOpException(String message)
  {
    super(message);
  }

  static final class NetworkException extends SyncOpException
  {}

  static final class AuthExpiredException extends SyncOpException
  {}

  static final class UnexpectedException extends SyncOpException
  {
    public UnexpectedException(String message)
    {
      super(message);
    }
    public UnexpectedException() {}
  }
}
