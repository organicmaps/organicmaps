package com.mapswithme.util.sharing;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;

import com.facebook.FacebookSdk;
import com.facebook.share.model.ShareLinkContent;
import com.facebook.share.widget.ShareDialog;
import com.mapswithme.maps.R;

import java.util.Locale;

public class PedestrianShareable extends TextShareable
{
  private static final String FACEBOOK_SHARE_URL = "http://maps.me/fb-pedestrian?lang=" + Locale.getDefault().getLanguage();

  public PedestrianShareable(Activity context)
  {
    super(context);
  }

  @Override
  public void share(SharingTarget target)
  {
    Activity activity = getActivity();
    Intent intent = getTargetIntent(target);
    String lowerCaseName = target.activityName.toLowerCase();

    if (lowerCaseName.contains("facebook"))
    {
      shareFacebook();
      return;
    }

    if (lowerCaseName.contains("mail"))
    {
      setSubject(R.string.share_walking_routes_email_subject);
      intent.putExtra(Intent.EXTRA_TEXT, activity.getString(R.string.share_walking_routes_email_body));
    }
    else if (lowerCaseName.contains("sms") || lowerCaseName.contains("mms"))
      TargetUtils.fillSmsIntent(activity, intent, activity.getString(R.string.share_walking_routes_sms));
    else
      setText(R.string.share_walking_routes_messenger);

    super.share(target);
  }

  private void shareFacebook()
  {
    FacebookSdk.sdkInitialize(getActivity());
    ShareDialog shareDialog = new ShareDialog(getActivity());
    if (ShareDialog.canShow(ShareLinkContent.class))
    {
      ShareLinkContent linkContent = new ShareLinkContent.Builder()
          .setContentUrl(Uri.parse(FACEBOOK_SHARE_URL))
          .build();

      shareDialog.show(linkContent);
    }
  }
}
