package app.organicmaps.test;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.res.AssetFileDescriptor;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.OpenableColumns;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

public class BookmarkImportProvider extends ContentProvider
{
  private static final byte[] GPX =
      ("<?xml version=\"1.0\"?><gpx version=\"1.1\" creator=\"Organic Maps test\" "
       + "xmlns=\"http://www.topografix.com/GPX/1/1\"><trk><name>Import test</name><trkseg>"
       + "<trkpt lat=\"51.0\" lon=\"19.0\"/><trkpt lat=\"51.1\" lon=\"19.1\"/>"
       + "</trkseg></trk></gpx>")
          .getBytes(StandardCharsets.UTF_8);
  // Passes preparation by its .gpx name, then fails native parsing on the mismatched closing tag.
  private static final byte[] MALFORMED_GPX =
      "<?xml version=\"1.0\"?><gpx version=\"1.1\"><trk></gpx>".getBytes(StandardCharsets.UTF_8);

  @NonNull
  private static byte[] contentFor(@NonNull Uri uri)
  {
    return uri.getPathSegments().get(0).equals("malformed") ? MALFORMED_GPX : GPX;
  }

  @Override
  public boolean onCreate()
  {
    return true;
  }

  @NonNull
  @Override
  public Cursor query(@NonNull Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder)
  {
    MatrixCursor cursor = new MatrixCursor(new String[] {OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE});
    cursor.addRow(new Object[] {uri.getLastPathSegment(), contentFor(uri).length});
    return cursor;
  }

  @NonNull
  @Override
  public String getType(@NonNull Uri uri)
  {
    return "application/gpx+xml";
  }

  @Nullable
  @Override
  public AssetFileDescriptor openAssetFile(@NonNull Uri uri, @NonNull String mode) throws FileNotFoundException
  {
    if (uri.getPathSegments().get(0).equals("null"))
      return null;

    byte[] content = contentFor(uri);
    File source = new File(getContext().getCacheDir(), "bookmark-import-provider.gpx");
    try (FileOutputStream out = new FileOutputStream(source))
    {
      out.write(content);
    }
    catch (IOException e)
    {
      throw new FileNotFoundException(e.toString());
    }
    return new AssetFileDescriptor(ParcelFileDescriptor.open(source, ParcelFileDescriptor.MODE_READ_ONLY), 0,
                                   content.length);
  }

  @Nullable
  @Override
  public Uri insert(@NonNull Uri uri, ContentValues values)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public int delete(@NonNull Uri uri, String selection, String[] selectionArgs)
  {
    throw new UnsupportedOperationException();
  }

  @Override
  public int update(@NonNull Uri uri, ContentValues values, String selection, String[] selectionArgs)
  {
    throw new UnsupportedOperationException();
  }
}
