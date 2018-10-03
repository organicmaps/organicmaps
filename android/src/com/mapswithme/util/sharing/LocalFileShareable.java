package com.mapswithme.util.sharing;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.support.annotation.Nullable;
import android.support.v4.content.FileProvider;

import com.mapswithme.maps.BuildConfig;

import java.io.File;

public class LocalFileShareable extends BaseShareable
{
  private final String mFileName;
  private final String mMimeType;

  LocalFileShareable(Activity context, String fileName, String mimeType)
  {
    super(context);
    mFileName = fileName;
    mMimeType = mimeType;
  }

  @Override
  protected void modifyIntent(Intent intent, @Nullable SharingTarget target)
  {
    super.modifyIntent(intent, target);
    Uri fileUri = FileProvider.getUriForFile(getActivity(), BuildConfig.FILE_PROVIDER_AUTHORITY,
                                             new File(mFileName));
    intent.putExtra(android.content.Intent.EXTRA_STREAM, fileUri);
  }

  @Override
  public String getMimeType()
  {
    return mMimeType;
  }
}
