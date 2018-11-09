package com.mapswithme.util.sharing;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

public final class TargetUtils
{
  public static final String TYPE_TEXT_PLAIN = "text/plain";
  static final String TYPE_MESSAGE_RFC822 = "message/rfc822";
  static final String EXTRA_SMS_BODY = "sms_body";
  static final String EXTRA_SMS_TEXT = Intent.EXTRA_TEXT;
  static final String URI_STRING_SMS = "sms:";

  private TargetUtils() {}

  public static void fillSmsIntent(Intent smsIntent, String body)
  {
    smsIntent.setData(Uri.parse(URI_STRING_SMS));
    smsIntent.putExtra(EXTRA_SMS_BODY, body);
  }

  @Nullable
  public static Intent makeAppSettingsLocationIntent(@NonNull Context context)
  {
    Intent intent = new Intent(android.provider.Settings.ACTION_LOCATION_SOURCE_SETTINGS);
    if (intent.resolveActivity(context.getPackageManager()) != null)
      return intent;

    intent = new Intent(android.provider.Settings.ACTION_SECURITY_SETTINGS);
    return intent.resolveActivity(context.getPackageManager()) == null ? null : intent;
  }
}
