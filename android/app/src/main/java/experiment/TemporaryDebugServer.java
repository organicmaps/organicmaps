package experiment;

import static org.nanohttpd.protocols.http.response.Response.newFixedLengthResponse;

import android.app.Activity;
import android.app.Application;
import android.os.Build;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.nanohttpd.protocols.http.IHTTPSession;
import org.nanohttpd.protocols.http.NanoHTTPD;
import org.nanohttpd.protocols.http.response.Response;

import app.organicmaps.bookmarks.data.BookmarkManager;

import java.io.IOException;

public class TemporaryDebugServer extends NanoHTTPD
{
  public TemporaryDebugServer(Activity act) throws IOException
  {
    super(8989);
    start(NanoHTTPD.SOCKET_READ_TIMEOUT, false);
    System.out.println("\nRunning! Point your browsers to http://localhost:8080/ \n");
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q)
    {
      act.registerActivityLifecycleCallbacks(new Application.ActivityLifecycleCallbacks()
      {
        @Override
        public void onActivityCreated(@NonNull Activity activity, @Nullable Bundle savedInstanceState)
        {

        }

        @Override
        public void onActivityStarted(@NonNull Activity activity)
        {

        }

        @Override
        public void onActivityResumed(@NonNull Activity activity)
        {

        }

        @Override
        public void onActivityPaused(@NonNull Activity activity)
        {

        }

        @Override
        public void onActivityStopped(@NonNull Activity activity)
        {

        }

        @Override
        public void onActivitySaveInstanceState(@NonNull Activity activity, @NonNull Bundle outState)
        {

        }

        @Override
        public void onActivityDestroyed(@NonNull Activity activity)
        {
          TemporaryDebugServer.this.stop();
        }
      });
    }
  }

  @Override
  public Response serve(IHTTPSession session)
  {
    if (session.getUri().startsWith("/callOnBookmarksChanged"))
    {
      new android.os.Handler(android.os.Looper.getMainLooper()).post(BookmarkManager.INSTANCE::onBookmarksChanged);

      return newFixedLengthResponse("done!!");
    }
    else if (session.getUri().startsWith("/callNativeLoadBookmarks"))
    {
      new android.os.Handler(android.os.Looper.getMainLooper()).post(BookmarkManager::loadBookmarks);

      return newFixedLengthResponse("done!!");
    }
    else if (session.getUri().startsWith("/callNativeReloadBookmark"))
    {
      String fp = session.getParms().get("filePath");
      if (fp == null)
        fp = "null";
      String filePath = fp;
      android.util.Log.e("debugserver", "filePath: " + filePath);
      new android.os.Handler(android.os.Looper.getMainLooper()).post(() -> BookmarkManager.nativeReloadBookmark(filePath));

      return newFixedLengthResponse("done!!");
    }
    else
    {
      return newFixedLengthResponse("unrecognized command :/");
    }
  }
}
