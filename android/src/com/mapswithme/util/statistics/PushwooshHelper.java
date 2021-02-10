package com.mapswithme.util.statistics;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;


public final class PushwooshHelper
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);

  public void sendTags(@NonNull String tag, @Nullable String[] params)
  {
    //TODO: move notifylib code to another place when Pushwoosh is deleted.
    if (params == null)
      return;


  }


  public static native void nativeProcessFirstLaunch();
  public static native void nativeSendEditorAddObjectTag();
  public static native void nativeSendEditorEditObjectTag();
  public static native @NonNull String nativeGetFormattedTimestamp();
}
