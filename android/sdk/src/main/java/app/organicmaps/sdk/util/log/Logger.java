package app.organicmaps.sdk.util.log;

import android.util.Log;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.BuildConfig;
import app.organicmaps.sdk.util.Assert;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

/// Thread-safe
public final class Logger
{
  private static final String TAG = Logger.class.getSimpleName();
  private static final String CORE_TAG = "OMcore";
  private static final String FILENAME = "app.log";
  private static final LogFileWriter FILE_WRITER = new LogFileWriter();

  public static void v(String tag)
  {
    logJava(Log.VERBOSE, tag, "", null);
  }

  public static void v(String tag, String msg)
  {
    logJava(Log.VERBOSE, tag, msg, null);
  }

  public static void v(String tag, String msg, Throwable tr)
  {
    logJava(Log.VERBOSE, tag, msg, tr);
  }

  public static void d(String tag)
  {
    logJava(Log.DEBUG, tag, "", null);
  }

  public static void d(String tag, String msg)
  {
    logJava(Log.DEBUG, tag, msg, null);
  }

  public static void d(String tag, String msg, Throwable tr)
  {
    logJava(Log.DEBUG, tag, msg, tr);
  }

  public static void i(String tag)
  {
    logJava(Log.INFO, tag, "", null);
  }

  public static void i(String tag, String msg)
  {
    logJava(Log.INFO, tag, msg, null);
  }

  public static void i(String tag, String msg, Throwable tr)
  {
    logJava(Log.INFO, tag, msg, tr);
  }

  public static void w(String tag, String msg)
  {
    logJava(Log.WARN, tag, msg, null);
  }

  public static void w(String tag, String msg, Throwable tr)
  {
    logJava(Log.WARN, tag, msg, tr);
  }

  public static void e(String tag, String msg)
  {
    logJava(Log.ERROR, tag, msg, null);
  }

  public static void e(String tag, String msg, Throwable tr)
  {
    logJava(Log.ERROR, tag, msg, tr);
  }

  @NonNull
  private static String getSourcePoint()
  {
    final StackTraceElement[] stackTrace = new Throwable().getStackTrace();
    // getSourcePoint() -> logImpl() -> logJava() -> Logger.x() -> caller.
    final int callerFrame = 4;
    if (stackTrace.length <= callerFrame)
      return "Unknown";
    final StackTraceElement st = stackTrace[callerFrame];
    StringBuilder sb = new StringBuilder(80);
    final String fileName = st.getFileName();
    if (fileName != null)
    {
      sb.append(fileName);
      final int lineNumber = st.getLineNumber();
      if (lineNumber >= 0)
        sb.append(':').append(lineNumber);
      sb.append(' ');
    }
    sb.append(st.getMethodName()).append("()");
    return sb.toString();
  }

  private static void logJava(int level, @Nullable String tag, @NonNull String msg, @Nullable Throwable tr)
  {
    logImpl(level, tag, msg, tr, true /* addJavaSourcePoint */);
  }

  // Also called from JNI to proxy native code logging (with tag == null).
  @Keep
  private static void log(int level, @Nullable String tag, @NonNull String msg, @Nullable Throwable tr)
  {
    logImpl(level, tag, msg, tr, false /* addJavaSourcePoint */);
  }

  private static void logImpl(int level, @Nullable String tag, @NonNull String msg, @Nullable Throwable tr,
                              boolean addJavaSourcePoint)
  {
    final String logsFolder = LogsManager.INSTANCE.getEnabledLogsFolder();

    if (logsFolder != null || BuildConfig.DEBUG || level >= Log.INFO)
    {
      final StringBuilder sb = new StringBuilder(180);
      // Add source point info for file logging, debug builds and ERRORs if its not from core.
      if (addJavaSourcePoint && tag != null && (logsFolder != null || BuildConfig.DEBUG || level == Log.ERROR))
        sb.append(getSourcePoint()).append(": ");
      sb.append(msg);
      if (tr != null)
        sb.append('\n').append(Log.getStackTraceString(tr));
      if (tag == null)
        tag = CORE_TAG;

      final String threadName = "(" + Thread.currentThread().getName() + ") ";
      if (logsFolder == null || BuildConfig.DEBUG)
        Log.println(level, tag, threadName + sb.toString());

      if (logsFolder != null)
      {
        sb.insert(0, String.valueOf(getLevelChar(level)) + '/' + tag + ": ");
        final String data = threadName + sb.toString();
        if (level >= Log.ERROR)
          FILE_WRITER.writeSync(logsFolder, data);
        else
          FILE_WRITER.writeAsync(logsFolder, level, data);
      }
    }
  }

  private static char getLevelChar(int level)
  {
    switch (level)
    {
    case Log.VERBOSE: return 'V';
    case Log.DEBUG: return 'D';
    case Log.INFO: return 'I';
    case Log.WARN: return 'W';
    case Log.ERROR: return 'E';
    }
    Assert.always(false, "Unknown log level " + level);
    return '_';
  }

  static void flushFileLogs()
  {
    FILE_WRITER.flush();
  }

  static void closeFileLogs()
  {
    FILE_WRITER.close();
  }

  private static class LogEntry
  {
    @Nullable
    final String mLogsFolder;
    @NonNull
    private final String mData;
    @Nullable
    private final CountDownLatch mFlushLatch;

    private LogEntry(@NonNull String logsFolder, @NonNull String data)
    {
      mLogsFolder = logsFolder;
      mData = data;
      mFlushLatch = null;
    }

    private LogEntry(@NonNull CountDownLatch flushLatch)
    {
      mLogsFolder = null;
      mData = "";
      mFlushLatch = flushLatch;
    }

    boolean isFlush()
    {
      return mFlushLatch != null;
    }
  }

  private static class LogFileWriter
  {
    private static final int MAX_SIZE = 3000000;
    private static final int MAX_LOG_FILES = 10;
    private static final int QUEUE_CAPACITY = 4096;
    private static final int MAX_BATCH_SIZE = 256;
    private static final int FLUSH_TIMEOUT_SECONDS = 2;

    @NonNull
    private final ArrayBlockingQueue<LogEntry> mQueue = new ArrayBlockingQueue<>(QUEUE_CAPACITY);
    @NonNull
    private final AtomicInteger mDroppedCount = new AtomicInteger();
    @NonNull
    private final Object mWriteLock = new Object();
    @NonNull
    private final SimpleDateFormat mDateFormat = new SimpleDateFormat("MM-dd HH:mm:ss.SSS", Locale.US);
    @Nullable
    private BufferedWriter mWriter;
    @Nullable
    private String mFilePath;
    private long mCurrentSize;

    LogFileWriter()
    {
      final Thread thread = new Thread(this::loop, "LoggerFileWriter");
      thread.setDaemon(true);
      thread.start();
    }

    void writeAsync(@NonNull String logsFolder, int level, @NonNull String data)
    {
      final LogEntry entry = new LogEntry(logsFolder, data);
      if (mQueue.offer(entry))
        return;

      if (level >= Log.WARN)
      {
        if (mQueue.poll() != null)
          mDroppedCount.incrementAndGet();
        if (mQueue.offer(entry))
          return;
      }

      mDroppedCount.incrementAndGet();
    }

    void writeSync(@NonNull String logsFolder, @NonNull String data)
    {
      synchronized (mWriteLock)
      {
        // Error logs bypass the async queue so native CHECK/ASSERT messages are persisted
        // before aborting, even if older debug/info logs are still waiting in the queue.
        writeDroppedCountLocked(logsFolder);
        writeLineLocked(logsFolder, data);
        flushLocked();
      }
    }

    void flush()
    {
      final CountDownLatch flushLatch = new CountDownLatch(1);
      final LogEntry flushEntry = new LogEntry(flushLatch);
      try
      {
        boolean enqueued = mQueue.offer(flushEntry);
        if (!enqueued)
        {
          if (mQueue.poll() != null)
            mDroppedCount.incrementAndGet();
          enqueued = mQueue.offer(flushEntry, FLUSH_TIMEOUT_SECONDS, TimeUnit.SECONDS);
        }

        // Max wait time here is 2 * FLUSH_TIMEOUT_SECONDS = 4 seconds + time for flushLocked.
        // Still reasonable for UI Settings.
        if (!enqueued || !flushLatch.await(FLUSH_TIMEOUT_SECONDS, TimeUnit.SECONDS))
        {
          Log.e(TAG, "Timed out waiting for the file logger flush request");
          synchronized (mWriteLock)
          {
            flushLocked();
          }
        }
      }
      catch (InterruptedException e)
      {
        Thread.currentThread().interrupt();
        Log.e(TAG, "Interrupted while flushing file logs", e);
      }
    }

    void close()
    {
      synchronized (mWriteLock)
      {
        mQueue.clear();
        closeWriterLocked();
      }
    }

    private void loop()
    {
      while (true)
      {
        try
        {
          processEntry(mQueue.take());
          for (int i = 0; i < MAX_BATCH_SIZE; ++i)
          {
            final LogEntry entry = mQueue.poll();
            if (entry == null)
              break;
            processEntry(entry);
            if (entry.isFlush())
              break;
          }
          synchronized (mWriteLock)
          {
            flushLocked();
          }
        }
        catch (InterruptedException e)
        {
          Thread.currentThread().interrupt();
          return;
        }
      }
    }

    private void processEntry(@NonNull LogEntry entry)
    {
      synchronized (mWriteLock)
      {
        if (entry.isFlush())
        {
          flushLocked();
          entry.mFlushLatch.countDown();
          return;
        }

        if (entry.mLogsFolder == null)
          return;

        writeDroppedCountLocked(entry.mLogsFolder);
        writeLineLocked(entry.mLogsFolder, entry.mData);
      }
    }

    private void writeDroppedCountLocked(@NonNull String logsFolder)
    {
      final int droppedCount = mDroppedCount.getAndSet(0);
      if (droppedCount == 0)
        return;

      writeLineLocked(logsFolder, "(LoggerFileWriter) W/" + TAG + ": Dropped " + droppedCount
                                      + " log lines because the file logger queue was full");
    }

    private void writeLineLocked(@NonNull String logsFolder, @NonNull String data)
    {
      final String line = mDateFormat.format(new Date()) + " " + data + "\n";
      try
      {
        ensureWriterLocked(logsFolder, line.length());
        if (mWriter == null)
          return;

        mWriter.write(line);
        mCurrentSize += line.length();
      }
      catch (IOException e)
      {
        Log.e(TAG, "Failed to write to " + getLogFile(logsFolder, 0).getPath() + ": " + data, e);
        closeWriterLocked();
      }
    }

    private void ensureWriterLocked(@NonNull String logsFolder, int nextLineSize) throws IOException
    {
      final File file = getLogFile(logsFolder, 0);
      final String filePath = file.getPath();
      if (!filePath.equals(mFilePath))
      {
        closeWriterLocked();
        mFilePath = filePath;
      }

      if (mWriter != null && mCurrentSize + nextLineSize <= MAX_SIZE)
        return;

      if (mWriter != null)
        rotateLocked(logsFolder);
      else if (file.exists() && file.length() > MAX_SIZE)
        rotateLocked(logsFolder);

      final boolean writeSystemInfo = !file.exists() || file.length() == 0;
      mWriter = new BufferedWriter(new FileWriter(file, true));
      mCurrentSize = file.length();

      if (writeSystemInfo)
      {
        final String systemInformation = LogsManager.INSTANCE.getSystemInformation();
        mWriter.write(systemInformation);
        mCurrentSize += systemInformation.length();
      }
    }

    private void rotateLocked(@NonNull String logsFolder)
    {
      closeWriterLocked();

      final File oldestFile = getLogFile(logsFolder, MAX_LOG_FILES - 1);
      if (oldestFile.exists() && !oldestFile.delete())
        Log.e(TAG, "Failed to delete old log file " + oldestFile.getPath());

      for (int i = MAX_LOG_FILES - 2; i >= 0; --i)
      {
        final File from = getLogFile(logsFolder, i);
        if (!from.exists())
          continue;

        final File to = getLogFile(logsFolder, i + 1);
        if (!from.renameTo(to))
          Log.e(TAG, "Failed to rotate log file " + from.getPath() + " to " + to.getPath());
      }

      mFilePath = getLogFile(logsFolder, 0).getPath();
      mCurrentSize = 0;
    }

    private void flushLocked()
    {
      if (mWriter == null)
        return;

      try
      {
        mWriter.flush();
      }
      catch (IOException e)
      {
        Log.e(TAG, "Failed to flush file " + mFilePath, e);
        closeWriterLocked();
      }
    }

    private void closeWriterLocked()
    {
      if (mWriter == null)
        return;

      try
      {
        mWriter.close();
      }
      catch (IOException e)
      {
        Log.e(TAG, "Failed to close file " + mFilePath, e);
      }
      finally
      {
        mWriter = null;
        mFilePath = null;
        mCurrentSize = 0;
      }
    }

    @NonNull
    private File getLogFile(@NonNull String logsFolder, int index)
    {
      if (index == 0)
        return new File(logsFolder, FILENAME);
      return new File(logsFolder, FILENAME + "." + index);
    }
  }
}
