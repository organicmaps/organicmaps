package app.organicmaps.sync.nc;

import app.organicmaps.bookmarks.data.BookmarkManager;

// TODO when working on the actual project (if selected) refactor out such classes into interfaces for testing
public class LocalFileUtils
{
  public static void deleteFileSafe(String filePath)
  {
    BookmarkManager.nativeDeleteBmCategoryPermanently(filePath);
  }

  public static void reloadBookmarksList(String filePath)
  {
    BookmarkManager.nativeReloadBookmark(filePath);
  }
}
