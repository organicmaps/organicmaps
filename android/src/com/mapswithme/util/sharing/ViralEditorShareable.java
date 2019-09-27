package com.mapswithme.util.sharing;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import androidx.annotation.DrawableRes;
import androidx.annotation.Nullable;

import java.util.Locale;

import com.facebook.FacebookSdk;
import com.facebook.share.model.ShareLinkContent;
import com.facebook.share.widget.ShareDialog;
import com.mapswithme.util.UiUtils;

public class ViralEditorShareable extends BaseShareable
{
  private static final String FACEBOOK_SHARE_URL = "http://maps.me/fb-editor-v1?lang=" + Locale.getDefault().getLanguage();
  private static final String VIRAL_TAIL = " http://maps.me/im_get";

  private final Uri mUri;

  public ViralEditorShareable(Activity context, @DrawableRes int resId)
  {
    super(context);
    mUri = UiUtils.getUriToResId(context, resId);
  }

  @Override
  protected void modifyIntent(Intent intent, @Nullable SharingTarget target)
  {
    super.modifyIntent(intent, target);
    intent.putExtra(Intent.EXTRA_STREAM, mUri);
  }

  @Override
  public String getMimeType()
  {
    return TargetUtils.TYPE_TEXT_PLAIN;
  }

  @Override
  public void share(SharingTarget target)
  {
    Intent intent = getTargetIntent(target);
    String lowerCaseName = target.activityName.toLowerCase();

    if (lowerCaseName.contains("facebook"))
    {
      shareFacebook();
      return;
    }

    setText(mText + VIRAL_TAIL);

    if (lowerCaseName.contains("sms") || lowerCaseName.contains("mms"))
      TargetUtils.fillSmsIntent(intent, mText);
    else if (lowerCaseName.contains("twitter"))
      setSubject("");
    else if (!lowerCaseName.contains("mail"))
    {
      setText(mSubject + "\n" + mText);
      setSubject("");
    }

    intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);

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
