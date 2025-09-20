package app.organicmaps.sdk.sync;

import java.util.HashMap;
import java.util.HashSet;

/**
 * A record to represent the state of bookmark files
 * @param omBookmarkFiles The filename-to-checksum map of kml files that were uploaded by Organic Maps itself.
 *                        None of the keys should be null. None of the values should be null, either.
 * @param userUploadedFiles List of kml/gpx/kmb/kmz filenames uploaded to the cloud directory by the user
 */
public record CloudFilesState(HashMap<String, String> omBookmarkFiles, HashSet<String> userUploadedFiles)
{
  public boolean doesFileExist(String fileName)
  {
    return omBookmarkFiles().containsKey(fileName) || userUploadedFiles().contains(fileName);
  }
}
