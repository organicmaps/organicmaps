package com.mapswithme.util.sharing;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.provider.Telephony;

final class TargetUtils
{
  static final String TYPE_MESSAGE_RFC822 = "message/rfc822";
  static final String TYPE_TEXT_PLAIN = "text/plain";
  static final String EXTRA_SMS_BODY = "sms_body";
  static final String EXTRA_SMS_TEXT = Intent.EXTRA_TEXT;
  static final String URI_STRING_SMS = "sms:";

  private TargetUtils() {}

  public static void fillSmsIntent(Activity activity, Intent smsIntent, String body)
  {
    smsIntent.setData(Uri.parse(URI_STRING_SMS));
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT)
    {
      final String defaultSms = Telephony.Sms.getDefaultSmsPackage(activity);
      smsIntent.setType(TYPE_TEXT_PLAIN);
      smsIntent.putExtra(EXTRA_SMS_TEXT, body);
      if (defaultSms != null)
        smsIntent.setPackage(defaultSms);
    }
    else
      smsIntent.putExtra(EXTRA_SMS_BODY, body);
  }
}
