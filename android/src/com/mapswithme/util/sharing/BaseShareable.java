package com.mapswithme.util.sharing;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import android.text.TextUtils;

import com.mapswithme.util.statistics.AlohaHelper;

abstract class BaseShareable
{
  private final Activity mActivity;
  protected Intent mBaseIntent;
  protected String mText;
  protected String mSubject;

  public BaseShareable(Activity activity)
  {
    mActivity = activity;
    mBaseIntent = new Intent(Intent.ACTION_SEND);
  }

  public Activity getActivity()
  {
    return mActivity;
  }

  protected void modifyIntent(Intent intent, @Nullable SharingTarget target) {}

  protected Intent getBaseIntent()
  {
    return mBaseIntent;
  }

  public Intent getTargetIntent(@Nullable SharingTarget target)
  {
    Intent res = getBaseIntent();

    if (!TextUtils.isEmpty(mText))
      res.putExtra(Intent.EXTRA_TEXT, mText);

    if (!TextUtils.isEmpty(mSubject))
      res.putExtra(Intent.EXTRA_SUBJECT, mSubject);

    String mime = getMimeType();
    if (!TextUtils.isEmpty(mime))
      res.setType(mime);

    modifyIntent(res, target);

    return res;
  }

  public void share(SharingTarget target)
  {
    Intent intent = getTargetIntent(target);
    target.setupComponentName(intent);

    try
    {
      mActivity.startActivity(intent);
    } catch (ActivityNotFoundException ignored)
    {
      AlohaHelper.logException(ignored);
    }
  }

  public BaseShareable setBaseIntent(Intent intent)
  {
    mBaseIntent = intent;
    return this;
  }

  public BaseShareable setText(String text)
  {
    mText = text;
    return this;
  }

  public BaseShareable setSubject(String subject)
  {
    mSubject = subject;
    return this;
  }

  public BaseShareable setText(@StringRes int textRes)
  {
    mText = getActivity().getString(textRes);
    return this;
  }

  public BaseShareable setSubject(@StringRes int subjectRes)
  {
    mSubject = getActivity().getString(subjectRes);
    return this;
  }

  public abstract String getMimeType();
}
