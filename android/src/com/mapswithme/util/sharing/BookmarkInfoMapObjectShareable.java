package com.mapswithme.util.sharing;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapPoint;
import com.mapswithme.maps.widget.placepage.Sponsored;
import com.mapswithme.util.statistics.Statistics;

import java.util.Arrays;

class BookmarkInfoMapObjectShareable<T extends MapPoint> extends BaseShareable
{
  private static final String DELIMITER = "\n";
  @NonNull
  private final T mItem;

  public BookmarkInfoMapObjectShareable(@NonNull Activity activity, @NonNull T item,
                                        @Nullable Sponsored sponsored)
  {
    super(activity);
    mItem = item;
    setSubject(R.string.bookmark_share_email_subject);

    String text = makeEmailBody(activity, sponsored, getEmailBodyContent());
    setText(text);
  }

  @NonNull
  private static String makeEmailBody(@NonNull Activity activity, @Nullable Sponsored sponsored,
                                      @NonNull Iterable<String> emailBodyContent)
  {
    String text = TextUtils.join(DELIMITER, emailBodyContent);

    if (sponsored != null && sponsored.getType() == Sponsored.TYPE_BOOKING)
      text = concatSponsoredText(activity, sponsored, text);

    return text;
  }

  @NonNull
  private static String concatSponsoredText(@NonNull Activity activity, @NonNull Sponsored sponsored,
                                            @NonNull String src)
  {
    return TextUtils.join(DELIMITER, Arrays.asList(src, activity.getString(R.string.sharing_booking)))
            + sponsored.getUrl();
  }

  @NonNull
  protected Iterable<String> getEmailBodyContent()
  {
    return TextUtils.isEmpty(getItem().getAddress())
           ? Arrays.asList(getItem().getTitle(), getGeoUrl(), getHttpUrl())
           : Arrays.asList(getItem().getTitle(), getItem().getAddress(), getGeoUrl(), getHttpUrl());
  }

  @NonNull
  protected T getItem()
  {
    return mItem;
  }

  @NonNull
  protected final String getGeoUrl()
  {
    return Framework.nativeGetGe0Url(getItem().getLat(), getItem().getLon(),
                                     getItem().getScale(), getItem().getTitle());
  }

  @NonNull
  protected final String getHttpUrl()
  {
    return Framework.getHttpGe0Url(getItem().getLat(), getItem().getLon(),
                                   getItem().getScale(), getItem().getTitle());
  }


  @Override
  public String getMimeType()
  {
    return null;
  }

  @Override
  public void share(SharingTarget target)
  {
    super.share(target);
    Statistics.INSTANCE.trackPlaceShared(target.name);
  }
}
