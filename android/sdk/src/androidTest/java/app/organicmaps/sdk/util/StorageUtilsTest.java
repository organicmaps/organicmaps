package app.organicmaps.sdk.util;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.system.ErrnoException;
import android.system.OsConstants;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import java.io.IOException;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class StorageUtilsTest
{
  @Test
  public void outOfSpaceClassificationUsesErrno()
  {
    assertTrue(StorageUtils.isOutOfSpace(errnoFailure(OsConstants.ENOSPC)));
    assertTrue(StorageUtils.isOutOfSpace(errnoFailure(OsConstants.EDQUOT)));
    assertFalse(StorageUtils.isOutOfSpace(errnoFailure(OsConstants.EIO)));
  }

  private static IOException errnoFailure(int errno)
  {
    return new IOException("I/O failed", new ErrnoException("test", errno));
  }
}
