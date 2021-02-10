package com.mapswithme.util.sharing;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import androidx.annotation.DrawableRes;
import androidx.annotation.Nullable;

import com.mapswithme.util.UiUtils;

public class ViralEditorShareable extends BaseShareable
{
  private static final String VIRAL_TAIL = " https://omaps.app/im_get";

  public ViralEditorShareable(Activity context)
  {
    super(context);
  }

  @Override
  protected void modifyIntent(Intent intent, @Nullable SharingTarget target)
  {
    super.modifyIntent(intent, target);
//    intent.putExtra(Intent.EXTRA_STREAM);
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
}
