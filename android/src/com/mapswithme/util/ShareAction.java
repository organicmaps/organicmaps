
package com.mapswithme.util;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.view.Menu;
import android.view.MenuItem;

import com.mapswithme.maps.R;

import java.util.HashMap;
import java.util.Map;

public abstract class ShareAction
{
  public final static int ID_SMS = 0xfff1;
  public final static int ID_EMAIL = 0xfff2;
  public final static int ID_ANY = 0xffff;

  public final static SmsShareAction SMS_SHARE = new SmsShareAction();
  public final static EmailShareAction EMAIL_SHARE = new EmailShareAction();
  public final static AnyShareAction ANY_SHARE = new AnyShareAction();

  @SuppressLint("UseSparseArrays")
  public final static Map<Integer, ShareAction> ACTIONS = new HashMap<Integer, ShareAction>();

  protected final int mId;
  protected final int mNameResId;
  protected final Intent mBaseIntent;

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

  public int getNameResId()
  {
    return mNameResId;
  }

  @SuppressLint("NewApi")
  public MenuItem addToMenuIfSupported(Context context, Menu menu, boolean showAsAction)
  {
    if (isSupported(context))
    {
      final String name = context.getResources().getString(getNameResId());
      final MenuItem menuItem = menu.add(Menu.NONE, getId(), getId(), name);
      if (Utils.apiEqualOrGreaterThan(11) && showAsAction)
        menuItem.setShowAsAction(MenuItem.SHOW_AS_ACTION_IF_ROOM);
      return menuItem;
    }
    return null;
  }

  public boolean isSupported(Context context)
  {
    return Utils.isIntentSupported(context, getIntent());
  }

  public void shareWithText(Activity activity, String body, String subject)
  {
    final Intent intent = getIntent();
    intent.putExtra(Intent.EXTRA_TEXT, body)
          .putExtra(Intent.EXTRA_SUBJECT, subject);

    activity.startActivity(intent);
  }

  public static class SmsShareAction extends ShareAction
  {

    protected SmsShareAction()
    {
      super(ID_SMS, R.string.message, new Intent(Intent.ACTION_VIEW).setData(Uri.parse("sms:")));
    }

    @Override
    public void shareWithText(Activity activity, String body, String subject)
    {
      final Intent smsIntent = getIntent();
      smsIntent.putExtra("sms_body", body);
      activity.startActivity(smsIntent);
    }

  }

  public static class EmailShareAction extends ShareAction
  {

    protected EmailShareAction()
    {
      super(ID_EMAIL, R.string.share_by_email, new Intent(Intent.ACTION_SEND).setType("message/rfc822"));
    }

  }

  public static class AnyShareAction extends ShareAction
  {

    protected AnyShareAction()
    {
      super(ID_ANY, R.string.share, new Intent(Intent.ACTION_SEND).setType("text/plain"));
    }

    @Override
    public void shareWithText(Activity activity, String body, String subject)
    {
      final Intent intent = getIntent();
      intent.putExtra(Intent.EXTRA_TEXT, body)
            .putExtra(Intent.EXTRA_SUBJECT, subject);
      final String header = activity.getString(R.string.share);
      activity.startActivity(Intent.createChooser(intent, header));
    }
  }

  static
  {
    ACTIONS.put(ID_ANY, ANY_SHARE);
    ACTIONS.put(ID_EMAIL, EMAIL_SHARE);
    ACTIONS.put(ID_SMS, SMS_SHARE);
  }

}
