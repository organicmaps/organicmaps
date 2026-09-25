package app.organicmaps.sdk.util;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import org.junit.Assume;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;

// These tests run on the JVM: isDirWritable() uses only java.io, and Logger calls are
// no-op stubs there (returnDefaultValues, see build.gradle).
public class StorageUtilsTest
{
  @Rule
  public TemporaryFolder tempFolder = new TemporaryFolder();

  @Test
  public void writableDir()
  {
    final File dir = tempFolder.getRoot();
    assertTrue(StorageUtils.isDirWritable(dir));
    // The probe must clean up after itself.
    assertFalse(new File(dir, "om_test_dir").exists());
    assertFalse(new File(dir, "om_test_file").exists());
  }

  @Test
  public void nonExistentDir()
  {
    assertFalse(StorageUtils.isDirWritable(new File(tempFolder.getRoot(), "missing")));
  }

  @Test
  public void fileInsteadOfDir() throws IOException
  {
    assertFalse(StorageUtils.isDirWritable(tempFolder.newFile()));
  }

  @Test
  public void leftoverProbeArtifactsAreReplaced() throws IOException
  {
    final File dir = tempFolder.getRoot();
    assertTrue(new File(dir, "om_test_dir").mkdir());
    final File staleFile = new File(dir, "om_test_file");
    try (FileOutputStream os = new FileOutputStream(staleFile))
    {
      os.write("stale content from a previous run".getBytes(StandardCharsets.UTF_8));
    }
    assertTrue(StorageUtils.isDirWritable(dir));
    assertFalse(staleFile.exists());
    assertFalse(new File(dir, "om_test_dir").exists());
  }

  @Test
  public void unwritableTestFile() throws IOException
  {
    final File dir = tempFolder.getRoot();
    // A non-empty directory in place of the probe file can neither be deleted nor written to.
    // The volume has free space, so this must be reported as not writable.
    final File blocker = new File(dir, "om_test_file");
    assertTrue(blocker.mkdir());
    assertTrue(new File(blocker, "child").createNewFile());
    assertFalse(StorageUtils.isDirWritable(dir));
  }

  @Test
  public void readOnlyDir()
  {
    final File dir = tempFolder.getRoot();
    // Running as root (some CI containers) ignores permission bits -- skip there.
    Assume.assumeTrue(dir.setWritable(false) && !dir.canWrite());
    try
    {
      assertFalse(StorageUtils.isDirWritable(dir));
    }
    finally
    {
      // Not asserted: a failure here would replace the real one reported by the test body.
      dir.setWritable(true);
    }
  }

  @Test
  public void readBackRequiresExactContent() throws IOException
  {
    final byte[] expected = "expected probe payload".getBytes(StandardCharsets.UTF_8);
    final File file = tempFolder.newFile();

    write(file, expected);
    assertTrue(StorageUtils.fileHasExpectedContent(file, expected));

    final byte[] different = expected.clone();
    different[different.length / 2] ^= 1;
    write(file, different);
    assertFalse(StorageUtils.fileHasExpectedContent(file, expected));

    write(file, Arrays.copyOf(expected, expected.length - 1));
    assertFalse(StorageUtils.fileHasExpectedContent(file, expected));

    final ByteArrayOutputStream longer = new ByteArrayOutputStream();
    longer.write(expected);
    longer.write(0);
    write(file, longer.toByteArray());
    assertFalse(StorageUtils.fileHasExpectedContent(file, expected));
  }

  private static void write(File file, byte[] content) throws IOException
  {
    try (FileOutputStream stream = new FileOutputStream(file))
    {
      stream.write(content);
    }
  }
}
