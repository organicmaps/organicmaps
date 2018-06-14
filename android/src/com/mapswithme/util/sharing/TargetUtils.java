package com.mapswithme.util.sharing;

import android.content.Intent;
import android.net.Uri;

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
}
