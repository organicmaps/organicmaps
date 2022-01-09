package app.organicmaps.util;

import android.content.ClipData;
import android.content.Context;
import android.content.Intent;
import android.content.ComponentName;
import android.location.Location;
import android.net.Uri;
import android.text.TextUtils;

import androidx.annotation.NonNull;

import app.organicmaps.Framework;
import app.organicmaps.R;
import app.organicmaps.bookmarks.data.BookmarkInfo;
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.widget.placepage.CoordinatesFormat;

public class SharingUtils
{
  private static final String KMZ_MIME_TYPE = "application/vnd.google-earth.kmz";
  private static final String TEXT_MIME_TYPE = "text/plain";

  // This utility class has only static methods
  private SharingUtils()
  {
  }

  public static void shareMyLocation(@NonNull Context context, @NonNull Location loc)
  {
    // TODO: try to obtain the address of the current position.
    shareLocation(context, loc.getLatitude(), loc.getLongitude(), Framework.nativeGetDrawScale(), true, "", "");
  }

  public static void shareMapObject(@NonNull Context context, @NonNull MapObject object)
  {
    shareLocation(context, object.getLat(), object.getLon(), object.getScale(),
                  MapObject.isOfType(MapObject.MY_POSITION, object), object.getName(), object.getAddress());
  }

  public static void shareBookmark(@NonNull Context context, @NonNull BookmarkInfo bookmark)
  {
    shareLocation(context, bookmark.getLat(), bookmark.getLon(), bookmark.getScale(),
                  false, bookmark.getName(), bookmark.getAddress());
  }

  public static void shareLocation(@NonNull Context context, double lat, double lon, double zoomLevel,
                                   boolean isMyPosition, @NonNull String name, @NonNull String address)
  {
    Intent intent = new Intent(Intent.ACTION_SEND);
    intent.setType(TEXT_MIME_TYPE);

    String subject = "";
    if (isMyPosition)
    {
      subject = context.getString(R.string.share_my_position);
      name = subject;
    }
    else
    {
      if (TextUtils.isEmpty(name) || TextUtils.equals(name, context.getString(R.string.core_placepage_unknown_place)))
      {
        name = address;
        address = "";
      }
      subject = TextUtils.isEmpty(name) ? context.getString(R.string.share_coords_subject_default)
                                        : context.getString(R.string.share_coords_subject, name);
    }
    intent.putExtra(Intent.EXTRA_SUBJECT, subject);

    StringBuilder text = new StringBuilder();
    text.append(name);
    if (!TextUtils.isEmpty(address))
    {
      if (text.length() > 0) text.append(UiUtils.NEW_STRING_DELIMITER);
      text.append(address);
    }
    if (text.length() > 0) text.append(UiUtils.NEW_STRING_DELIMITER);
    text.append(Framework.getHttpGe0Url(lat, lon, zoomLevel, name));
    text.append(UiUtils.NEW_STRING_DELIMITER);
    text.append(Framework.nativeFormatLatLon(lat, lon, CoordinatesFormat.LatLonDecimal.getId()).replace(" ", ""));
    intent.putExtra(Intent.EXTRA_TEXT, text.toString());

    context.startActivity(Intent.createChooser(intent, context.getString(R.string.share)));
  }

  public static void shareBookmarkFile(Context context, String fileName)
  {
    Intent intent = new Intent(Intent.ACTION_SEND);

    final String subject = context.getString(R.string.share_bookmarks_email_subject);
    intent.putExtra(Intent.EXTRA_SUBJECT, subject);

    final String text = context.getString(R.string.share_bookmarks_email_body);
    intent.putExtra(Intent.EXTRA_TEXT, text);

    final Uri fileUri = StorageUtils.getUriForFilePath(context, fileName);
    intent.putExtra(android.content.Intent.EXTRA_STREAM, fileUri);
    // Properly set permissions for intent, see
    // https://developer.android.com/reference/androidx/core/content/FileProvider#include-the-permission-in-an-intent
    intent.setDataAndType(fileUri, KMZ_MIME_TYPE);
    intent.setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
    if (android.os.Build.VERSION.SDK_INT <= android.os.Build.VERSION_CODES.LOLLIPOP_MR1) {
      intent.setClipData(ClipData.newRawUri("", fileUri));
      intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
    }

    Intent chooser = Intent.createChooser(intent, context.getString(R.string.share));

    // Prevent sharing to ourselves (supported from API Level 24).
    if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.N)
    {
      ComponentName[] excludeSelf = { new ComponentName(context, app.organicmaps.SplashActivity.class) };
      chooser.putExtra(Intent.EXTRA_EXCLUDE_COMPONENTS, excludeSelf);
    }

    context.startActivity(chooser);
  }
}
