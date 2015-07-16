package com.mapswithme.util.sharing;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;

public class LocalFileShareable extends BaseShareable
{
  private final String mFileName;
  private final String mMimeType;

  public LocalFileShareable(Activity context, String fileName, String mimeType)
  {
    super(context);
    mFileName = fileName;
    mMimeType = mimeType;
  }

  @Override
  protected void modifyIntent(Intent intent)
  {
    super.modifyIntent(intent);
    intent.putExtra(android.content.Intent.EXTRA_STREAM, Uri.parse("file://" + mFileName));
  }

  @Override
  public String getMimeType()
  {
    return mMimeType;
  }
}
