package app.organicmaps.consumer;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.Intent;
import android.os.Bundle;
import android.os.SystemClock;
import android.util.Log;
import android.widget.TextView;

/**
 * Platform-only probe for the minified release APK. Sharing a third-party test runner's dependencies with a
 * separately optimized target can require extra keep rules, which would weaken this consumer check.
 * Resolve SDK/HTTP classes through the target loader so test-APK dependencies cannot satisfy a missing target type.
 */
public final class ConsumerTestRunner extends Instrumentation
{
  @Override
  public void onCreate(Bundle arguments)
  {
    super.onCreate(arguments);
    start();
  }

  @Override
  public void onStart()
  {
    Bundle status = new Bundle();
    status.putString("id", "InstrumentationTestRunner");
    status.putString("class", getClass().getName());
    status.putString("test", "initializesAndRendersFromPublishedAar");
    status.putInt("numtests", 1);
    status.putInt("current", 1);
    sendStatus(1, status);
    Activity activity = null;
    Throwable failure = null;
    try
    {
      // JNI uses this literal external return type and method name; target-app R8 must preserve both.
      ClassLoader loader = getTargetContext().getClassLoader();
      Class<?> http = loader.loadClass("app.organicmaps.sdk.util.HttpClient");
      Class<?> params = loader.loadClass("app.organicmaps.sdk.util.HttpClient$Params");
      Class<?> call = http.getDeclaredMethod("runAsync", params, long.class).getReturnType();
      if (!"okhttp3.Call".equals(call.getName()))
        throw new AssertionError("JNI HTTP return type was renamed to " + call.getName());
      if (call.getMethod("cancel").getReturnType() != void.class)
        throw new AssertionError("JNI HTTP cancellation must have descriptor ()V");

      Intent intent = new Intent().setClassName(getTargetContext(), "app.organicmaps.consumer.MainActivity");
      intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
      activity = startActivitySync(intent);
      Activity target = activity;
      long deadline = SystemClock.uptimeMillis() + 60000;
      boolean[] ready = {false};
      while (!ready[0] && SystemClock.uptimeMillis() < deadline)
      {
        runOnMainSync(() -> {
          TextView text = target.findViewById(android.R.id.text1);
          ready[0] = text != null && "SDK ready".contentEquals(text.getText());
        });
        if (!ready[0])
          SystemClock.sleep(100);
      }
      if (!ready[0])
        throw new AssertionError("The published SDK did not initialize its renderer");
    }
    catch (Throwable error)
    {
      failure = error;
    }
    finally
    {
      if (activity != null)
        runOnMainSync(activity::finish);
    }
    if (failure == null)
    {
      sendStatus(0, status);
      status.putString("stream", "\nOK (1 test)\n");
      finish(Activity.RESULT_OK, status);
    }
    else
    {
      status.putString("stack", Log.getStackTraceString(failure));
      sendStatus(-2, status);
      finish(Activity.RESULT_CANCELED, status);
    }
  }
}
