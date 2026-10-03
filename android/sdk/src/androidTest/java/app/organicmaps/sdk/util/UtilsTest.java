package app.organicmaps.sdk.util;

import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.system.ErrnoException;
import android.system.OsConstants;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import java.io.Closeable;
import java.io.IOException;
import java.util.concurrent.atomic.AtomicBoolean;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class UtilsTest
{
  @Test
  public void closeIgnoringEpermIgnoresNestedEperm() throws IOException
  {
    final AtomicBoolean closed = new AtomicBoolean();
    final Closeable file = () ->
    {
      closed.set(true);
      throw errnoFailure(OsConstants.EPERM);
    };
    Utils.closeIgnoringEperm(file, "test file");
    assertTrue(closed.get());
  }

  @Test
  public void closeIgnoringEpermPropagatesOtherErrnos()
  {
    final IOException failure = errnoFailure(OsConstants.EIO);
    final Closeable file = () ->
    {
      throw failure;
    };
    try
    {
      Utils.closeIgnoringEperm(file, "test file");
      fail("EIO must not be ignored");
    }
    catch (IOException e)
    {
      assertSame(failure, e);
    }
  }

  private static IOException errnoFailure(int errno)
  {
    return new IOException("I/O failed", new ErrnoException("test", errno));
  }
}
