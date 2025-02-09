package app.organicmaps.util;

import android.app.Activity;
import android.content.ClipData;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.ComponentName;
import android.location.Location;
import android.net.Uri;
import android.os.Build;
import android.text.TextUtils;
import android.util.Pair;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContract;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.Fragment;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import app.organicmaps.Framework;
import app.organicmaps.R;
import app.organicmaps.SplashActivity;
import app.organicmaps.bookmarks.data.BookmarkInfo;
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.util.log.Logger;

public class SharingUtils
{
  private static final String TAG = SharingUtils.class.getSimpleName();
  private static final String KML_MIME_TYPE = "application/vnd.google-earth.kml+xml";
  private static final String KMZ_MIME_TYPE = "application/vnd.google-earth.kmz";
  private static final String GPX_MIME_TYPE = "application/gpx+xml";
  private static final String TEXT_MIME_TYPE = "text/plain";
  public static class ShareInfo
  {
    public String mMimeType = "";
    public String mSubject = "";
    public String mText = "";
    public String mMail = "";
    public String mFileName = "";

    ShareInfo()
    {
    }

    ShareInfo(@NonNull String mimeType, String subject, String text, String mail, String fileName)
    {
      mMimeType = mimeType;
      mSubject = subject;
      mText = text;
      mMail = mail;
      mFileName = fileName;
    }
  }

  public static class SharingIntent
  {
    private final Intent mIntent;
    private Uri mSource;

    SharingIntent(@NonNull Intent intent, Uri source)
    {
      mIntent = intent;
      mSource = source;
    }

    SharingIntent(@NonNull Intent intent)
    {
      mIntent = intent;
    }

    public void SetSourceFile(@NonNull Uri source)
    {
      mSource = source;
    }

    Intent GetIntent() {return mIntent;}
    Uri GetSourceFile() {return mSource;}
  }

  public static class SharingContract extends ActivityResultContract<SharingIntent, Pair<Uri,Uri>>
  {
    static private Uri sourceUri;

    @NonNull
    @Override
    public Intent createIntent(@NonNull Context context, SharingIntent input)
    {
      sourceUri = input.GetSourceFile();
      return input.GetIntent();
    }

    @Override
    public Pair<Uri,Uri> parseResult(int resultCode, Intent intent)
    {
      if (resultCode == Activity.RESULT_OK && intent != null)
      {
        Uri dest = intent.getData();
        return new Pair<>(sourceUri, dest);
      }
      return null;
    }
  }

  // This utility class has only static methods
  private SharingUtils()
  {
  }

  public static void shareLocation(@NonNull Context context, @NonNull Location loc)
  {
    Intent intent = new Intent(Intent.ACTION_SEND);
    intent.setType(TEXT_MIME_TYPE);

    final String subject = context.getString(R.string.share);
    intent.putExtra(Intent.EXTRA_SUBJECT, subject);

    final String geoUrl = Framework.nativeGetGe0Url(loc.getLatitude(), loc.getLongitude(), Framework
        .nativeGetDrawScale(), "");
    final String coordUrl = String.format("https://omaps.app/%.5f,%.5f", loc.getLatitude(), loc.getLongitude());
    final String httpUrl = Framework.getHttpGe0Url(loc.getLatitude(), loc.getLongitude(), Framework
        .nativeGetDrawScale(), "");
    final String text = context.getString(R.string.my_position_share_sms, httpUrl , coordUrl); 
    intent.putExtra(Intent.EXTRA_TEXT, text);

    context.startActivity(Intent.createChooser(intent, context.getString(R.string.share)));
  }

  public static void shareMapObject(@NonNull Context context, @NonNull MapObject object)
  {
    Intent intent = new Intent(Intent.ACTION_SEND);
    intent.setType(TEXT_MIME_TYPE);

    final String subject = object.isMyPosition() ?
                           context.getString(R.string.my_position_share_email_subject) :
                           context.getString(R.string.bookmark_share_email_subject);
    intent.putExtra(Intent.EXTRA_SUBJECT, subject);

    final String geoUrl = Framework.nativeGetGe0Url(object.getLat(), object.getLon(),
                                                    object.getScale(), object.getName());
    final String coordUrl = String.format("https://omaps.app/%.5f,%.5f", object.getLat(), object.getLon());
    final String httpUrl = Framework.getHttpGe0Url(object.getLat(), object.getLon(),
                                                   object.getScale(), object.getName());
    final String address = TextUtils.isEmpty(object.getAddress()) ? object.getName() : object.getAddress();
    final String text = context.getString(R.string.my_position_share_email, address,httpUrl ,coordUrl );
    intent.putExtra(Intent.EXTRA_TEXT, text);

    context.startActivity(Intent.createChooser(intent, context.getString(R.string.share)));
  }

  public static void shareBookmark(@NonNull Context context, @NonNull BookmarkInfo bookmark)
  {
    Intent intent = new Intent(Intent.ACTION_SEND);
    intent.setType(TEXT_MIME_TYPE);

    final String subject = context.getString(R.string.bookmark_share_email_subject);
    intent.putExtra(Intent.EXTRA_SUBJECT, subject);

    final String geoUrl = Framework.nativeGetGe0Url(bookmark.getLat(), bookmark.getLon(),
                                                    bookmark.getScale(), bookmark.getName());
    final String coordUrl = String.format("https://omaps.app/%.5f,%.5f", bookmark.getLat(), bookmark.getLon());

    final String httpUrl = Framework.getHttpGe0Url(bookmark.getLat(), bookmark.getLon(),
                                                   bookmark.getScale(), bookmark.getName());
    StringBuilder text = new StringBuilder();
    text.append(bookmark.getName());
    if (!TextUtils.isEmpty(bookmark.getAddress()))
    {
      text.append(UiUtils.NEW_STRING_DELIMITER);
      text.append(bookmark.getAddress());
    }
    text.append(UiUtils.NEW_STRING_DELIMITER);
    text.append(httpUrl);
    text.append(UiUtils.NEW_STRING_DELIMITER);
    text.append(coordUrl);
    intent.putExtra(Intent.EXTRA_TEXT, text.toString());

    context.startActivity(Intent.createChooser(intent, context.getString(R.string.share)));
  }

  private static void ProcessShareResult(@NonNull ContentResolver resolver, Pair<Uri, Uri> result)
  {
    if (resolver!=null && result != null)
    {
      Uri sourceUri = result.first;
      Uri destinationUri = result.second;

      try
      {
        if (sourceUri != null && destinationUri != null)
          StorageUtils.copyFile(resolver, sourceUri, destinationUri);
      }
      catch (IOException e)
      {
        throw new RuntimeException(e);
      }
    }
  }
  public static ActivityResultLauncher<SharingIntent> RegisterLauncher(@NonNull Fragment fragment)
  {
    return fragment.registerForActivityResult(
      new SharingContract(),
      result -> ProcessShareResult(fragment.requireContext().getContentResolver(), result)
    );
  }
  public static ActivityResultLauncher<SharingIntent> RegisterLauncher(@NonNull AppCompatActivity activity)
  {
    return activity.registerForActivityResult(
      new SharingContract(),
      result -> ProcessShareResult(activity.getContentResolver(), result)
    );
  }

  public static void shareFile(Context context, ActivityResultLauncher<SharingIntent> launcher, ShareInfo info)
  {
    Intent intent = new Intent(Intent.ACTION_SEND_MULTIPLE);

    if (!info.mSubject.isEmpty())
      intent.putExtra(Intent.EXTRA_SUBJECT, info.mSubject);
    if (!info.mMail.isEmpty())
      intent.putExtra(Intent.EXTRA_EMAIL, new String[]{info.mMail});
    if (!info.mText.isEmpty())
      intent.putExtra(Intent.EXTRA_TEXT, info.mText);

    Intent chooser = Intent.createChooser(intent, context.getString(R.string.share));
    SharingIntent sharingIntent = new SharingIntent(chooser);

    if (!info.mFileName.isEmpty())
    {
      final Uri fileUri = StorageUtils.getUriForFilePath(context, info.mFileName);
      Logger.i(TAG, "Sharing file " + info.mMimeType + " " + info.mFileName + " with URI " + fileUri);
      intent.putParcelableArrayListExtra(Intent.EXTRA_STREAM, new ArrayList<>(List.of(fileUri)));
      intent.setDataAndType(fileUri, info.mMimeType);

      // Properly set permissions for intent, see
      // https://developer.android.com/reference/androidx/core/content/FileProvider#include-the-permission-in-an-intent
      intent.setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_ACTIVITY_NEW_TASK);

      if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.LOLLIPOP_MR1) {
        intent.setClipData(ClipData.newRawUri("", fileUri));
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION | Intent.FLAG_ACTIVITY_NEW_TASK);
      }

      Intent saveIntent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
      saveIntent.setType(info.mMimeType);

      final String fileName = fileUri.getPathSegments().get(fileUri.getPathSegments().size()-1);
      saveIntent.putExtra(Intent.EXTRA_TITLE, fileName);

      Intent[] extraIntents = {saveIntent};

      // Prevent sharing to ourselves (supported from API Level 24).
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
      {
        ComponentName[] excludeSelf = { new ComponentName(context, SplashActivity.class) };
        chooser.putExtra(Intent.EXTRA_EXCLUDE_COMPONENTS, excludeSelf);
      }

      chooser.putExtra(Intent.EXTRA_INITIAL_INTENTS, extraIntents);

      sharingIntent.SetSourceFile(fileUri);
    }

    launcher.launch(sharingIntent);
  }

  public static void shareBookmarkFile(Context context, ActivityResultLauncher<SharingIntent> launcher, String fileName, String mimeType)
  {
    final String subject = context.getString(R.string.share_bookmarks_email_subject);
    final String text = context.getString(R.string.share_bookmarks_email_body);

    ShareInfo info = new ShareInfo(mimeType, subject, text, "", fileName);
    shareFile(context, launcher, info);
  }
}
