package com.mapswithme.util.sharing;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.provider.Telephony;
import android.support.annotation.StringRes;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.MapObject.MapObjectType;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;

public abstract class ShareAction
{
  public static final SmsShareAction SMS_SHARE = new SmsShareAction();
  public static final EmailShareAction EMAIL_SHARE = new EmailShareAction();
  public static final AnyShareAction ANY_SHARE = new AnyShareAction();

  private static final String EXTRA_SMS_BODY = "sms_body";
  private static final String EXTRA_SMS_TEXT = Intent.EXTRA_TEXT;

  protected static final String TYPE_MESSAGE_RFC822 = "message/rfc822";
  protected static final String TYPE_TEXT_PLAIN = "text/plain";

  private static final String URI_STRING_SMS = "sms:";

  @StringRes
  protected final int mNameResId;
  protected final Intent mBaseIntent;

  protected ShareAction(int nameResId, Intent baseIntent)
  {
    mNameResId = nameResId;
    mBaseIntent = baseIntent;
  }

  public Intent getIntent()
  {
    return new Intent(mBaseIntent);
  }

  public boolean isSupported(Context context)
  {
    return Utils.isIntentSupported(context, getIntent());
  }

  public void shareMapObject(Activity activity, MapObject mapObject)
  {
    SharingHelper.shareOutside(new MapObjectShareable(activity, mapObject)
        .setBaseIntent(new Intent(mBaseIntent)), mNameResId);
  }

  public static class SmsShareAction extends ShareAction
  {
    protected SmsShareAction()
    {
      super(R.string.share_by_message, new Intent(Intent.ACTION_VIEW).setData(Uri.parse(URI_STRING_SMS)));
    }

    public void shareWithText(Activity activity, String body)
    {
      Intent smsIntent;
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT)
      {
        final String defaultSms = Telephony.Sms.getDefaultSmsPackage(activity);
        smsIntent = new Intent(Intent.ACTION_SEND);
        smsIntent.setType(TYPE_TEXT_PLAIN);
        smsIntent.putExtra(EXTRA_SMS_TEXT, body);
        if (defaultSms != null)
          smsIntent.setPackage(defaultSms);
      }
      else
      {
        smsIntent = getIntent();
        smsIntent.putExtra(EXTRA_SMS_BODY, body);
      }
      activity.startActivity(smsIntent);
    }

    @Override
    public void shareMapObject(Activity activity, MapObject mapObject)
    {
      final String ge0Url = Framework.nativeGetGe0Url(mapObject.getLat(), mapObject.getLon(), mapObject.getScale(), "");
      final String httpUrl = Framework.getHttpGe0Url(mapObject.getLat(), mapObject.getLon(), mapObject.getScale(), "");

      final int bodyId = mapObject.getType() == MapObjectType.MY_POSITION ? R.string.my_position_share_sms : R.string.bookmark_share_sms;

      final String body = activity.getString(bodyId, ge0Url, httpUrl);

      shareWithText(activity, body);

      Statistics.INSTANCE.trackPlaceShared(this.getClass().getSimpleName());
    }
  }

  public static class EmailShareAction extends ShareAction
  {
    protected EmailShareAction()
    {
      super(R.string.share_by_email, new Intent(Intent.ACTION_SEND).setType(TYPE_MESSAGE_RFC822));
    }
  }

  public static class AnyShareAction extends ShareAction
  {
    protected AnyShareAction()
    {
      super(R.string.share, new Intent(Intent.ACTION_SEND).setType(TYPE_TEXT_PLAIN));
    }

    public void share(final Activity activity, final String body)
    {
      SharingHelper.shareOutside(new TextShareable(activity, body));
    }
  }
}