package com.mapswithme.util.sharing;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.provider.Telephony;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.MapObject.MapObjectType;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;

import java.util.HashMap;
import java.util.Map;

public abstract class ShareAction
{
  public final static int ID_SMS = 0xfff1;
  public final static int ID_EMAIL = 0xfff2;
  public final static int ID_ANY = 0xffff;

  @SuppressLint("UseSparseArrays")
  public final static Map<Integer, ShareAction> ACTIONS = new HashMap<>();

  /* Actions*/
  private final static SmsShareAction SMS_SHARE = new SmsShareAction();
  private final static EmailShareAction EMAIL_SHARE = new EmailShareAction();
  private final static AnyShareAction ANY_SHARE = new AnyShareAction();

  /* Extras*/
  private static final String EXTRA_SMS_BODY = "sms_body";
  private static final String EXTRA_SMS_TEXT = Intent.EXTRA_TEXT;


  /* Types*/
  private static final String TYPE_MESSAGE_RFC822 = "message/rfc822";
  private static final String TYPE_TEXT_PLAIN = "text/plain";

  /* URIs*/
  private static final String URI_STRING_SMS = "sms:";

  protected final int mId;
  protected final int mNameResId;
  protected final Intent mBaseIntent;

  public static SmsShareAction getSmsShare()
  {
    return SMS_SHARE;
  }

  public static EmailShareAction getEmailShare()
  {
    return EMAIL_SHARE;
  }

  public static AnyShareAction getAnyShare()
  {
    return ANY_SHARE;
  }

  protected ShareAction(int id, int nameResId, Intent baseIntent)
  {
    mId = id;
    mNameResId = nameResId;
    mBaseIntent = baseIntent;
  }

  public Intent getIntent()
  {
    return new Intent(mBaseIntent);
  }

  public int getId()
  {
    return mId;
  }

  public boolean isSupported(Context context)
  {
    return Utils.isIntentSupported(context, getIntent());
  }


  /**
   * BASE share method
   */
  public void shareMapObject(Activity activity, MapObject mapObject)
  {
    SharingHelper.shareOutside(new MapObjectShareable(activity, mapObject)
                                   .setBaseIntent(mBaseIntent),
                               mNameResId);
  }

  /**
   * SMS
   */
  public static class SmsShareAction extends ShareAction
  {
    protected SmsShareAction()
    {
      super(ID_SMS, R.string.share_by_message, new Intent(Intent.ACTION_VIEW).setData(Uri.parse(URI_STRING_SMS)));
    }

    public void shareWithText(Activity activity, String body)
    {
      Intent smsIntent;
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT)
      {
        final String defaultSms = Telephony.Sms.getDefaultSmsPackage(activity);
        smsIntent = new Intent(Intent.ACTION_SEND);
        smsIntent.setType("text/plain");
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

  /**
   * EMAIL
   */
  public static class EmailShareAction extends ShareAction
  {
    protected EmailShareAction()
    {
      super(ID_EMAIL, R.string.share_by_email, new Intent(Intent.ACTION_SEND).setType(TYPE_MESSAGE_RFC822));
    }
  }

  /**
   * ANYTHING
   */
  public static class AnyShareAction extends ShareAction
  {
    protected AnyShareAction()
    {
      super(ID_ANY, R.string.share, new Intent(Intent.ACTION_SEND).setType(TYPE_TEXT_PLAIN));
    }

    public static void share(final Activity activity, final String body)
    {
      SharingHelper.shareOutside(new TextShareable(activity, body));
    }
  }

  static
  {
    ACTIONS.put(ID_ANY, getAnyShare());
    ACTIONS.put(ID_EMAIL, getEmailShare());
    ACTIONS.put(ID_SMS, getSmsShare());
  }
}