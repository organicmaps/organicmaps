package com.mapswithme.util.sharing;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.BookmarkInfo;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.widget.placepage.Sponsored;
import com.mapswithme.util.Utils;

public abstract class ShareOption
{
  @StringRes
  private final int mNameResId;
  private final Intent mBaseIntent;

  protected ShareOption(int nameResId, Intent baseIntent)
  {
    mNameResId = nameResId;
    mBaseIntent = baseIntent;
  }

  public boolean isSupported(Context context)
  {
    return Utils.isIntentSupported(context, mBaseIntent);
  }

  public void shareMapObject(Activity activity, @NonNull MapObject mapObject, @Nullable Sponsored sponsored)
  {
    MapObjectShareable shareable = new MapObjectShareable(activity, mapObject, sponsored);
    shareObjectInternal(shareable);
  }

  public void shareBookmarkObject(Activity activity, @NonNull BookmarkInfo mapObject,
                                  @Nullable Sponsored sponsored)
  {
    BookmarkInfoMapObjectShareable<BookmarkInfo> shareable =
        new BookmarkInfoMapObjectShareable<>(activity, mapObject, sponsored);
    shareObjectInternal(shareable);
  }

  private void shareObjectInternal(@NonNull BaseShareable shareable)
  {
    SharingHelper.shareOutside(shareable
                                   .setBaseIntent(new Intent(mBaseIntent)), mNameResId);
  }

  public static class EmailShareOption extends ShareOption
  {
    protected EmailShareOption()
    {
      super(R.string.share_by_email, new Intent(Intent.ACTION_SEND).setType(TargetUtils.TYPE_MESSAGE_RFC822));
    }
  }

  public static class AnyShareOption extends ShareOption
  {
    public static final AnyShareOption ANY = new AnyShareOption();

    protected AnyShareOption()
    {
      super(R.string.share, new Intent(Intent.ACTION_SEND).setType(TargetUtils.TYPE_TEXT_PLAIN));
    }

    public void share(Activity activity, String body)
    {
      SharingHelper.shareOutside(new TextShareable(activity, body));
    }

    public void share(Activity activity, String body, @StringRes int titleRes)
    {
      SharingHelper.shareOutside(new TextShareable(activity, body), titleRes);
    }
  }
}
