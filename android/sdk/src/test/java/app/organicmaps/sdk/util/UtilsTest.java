package app.organicmaps.sdk.util;

import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.io.Closeable;
import java.io.IOException;
import java.util.concurrent.atomic.AtomicBoolean;
import org.junit.Test;

public class UtilsTest
{
  @Test
  public void closeIgnoringEpermClosesResource() throws IOException
  {
    final AtomicBoolean closed = new AtomicBoolean();
    Utils.closeIgnoringEperm(() -> closed.set(true), "test resource");
    assertTrue(closed.get());
  }

  @Test
  public void closeIgnoringEpermPropagatesOtherIoErrors()
  {
    final IOException failure = new IOException("close failed");
    final Closeable resource = () ->
    {
      throw failure;
    };
    try
    {
      Utils.closeIgnoringEperm(resource, "test resource");
      fail("A non-EPERM close error must be propagated");
    }
    catch (IOException e)
    {
      assertSame(failure, e);
    }
  }
}
