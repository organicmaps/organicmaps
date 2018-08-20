package com.mapswithme.util.statistics;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.pushwoosh.Pushwoosh;
import com.pushwoosh.exception.PushwooshException;
import com.pushwoosh.function.Result;
import com.pushwoosh.tags.TagsBundle;
import ru.mail.libnotify.api.NotificationApi;

import java.util.Arrays;
import java.util.Collections;
import java.util.Map;

public final class PushwooshHelper
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  @NonNull
  private final NotificationApi mNotificationApi;

  public PushwooshHelper(@NonNull NotificationApi api)
  {
    mNotificationApi = api;
  }

  public void sendTags(@NonNull String tag, @Nullable String[] params)
  {
    //TODO: move notifylib code to another place when Pushwoosh is deleted.
    if (params == null)
      return;

    TagsBundle.Builder builder = new TagsBundle.Builder();
    boolean isSingleParam = params.length == 1;
    TagsBundle tagsBundle = isSingleParam ? builder.putString(tag, params[0]).build()
                                          : builder.putList(tag, Arrays.asList(params)).build();
    Pushwoosh.getInstance().sendTags(tagsBundle, this::onPostExecute);
    sendLibNotifyParams(tag, isSingleParam ? params[0] : params);
  }

  private void onPostExecute(@NonNull Result<Void, PushwooshException> result)
  {
    if (result.isSuccess())
      onSuccess(result);
    else
      onError(result);
  }

  private void onError(@NonNull Result<Void, PushwooshException> result)
  {
    PushwooshException exception = result.getException();
    String msg = exception == null ? null : exception.getLocalizedMessage();
    LOGGER.e("Pushwoosh", msg != null ? msg : "onSentTagsError");
  }

  private void onSuccess(@NonNull Result<Void, PushwooshException> result)
  {
    /* Do nothing by default */
  }

  private void sendLibNotifyParams(@NonNull String tag, @NonNull Object value)
  {
    Map<String, Object> map = Collections.singletonMap(tag, value);
    mNotificationApi.collectEventBatch(map);
  }

  public static native void nativeProcessFirstLaunch();
  public static native void nativeSendEditorAddObjectTag();
  public static native void nativeSendEditorEditObjectTag();
}
