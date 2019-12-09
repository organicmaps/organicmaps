package com.mapswithme.util.sharing;

import android.app.Activity;
import android.text.TextUtils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.placepage.Sponsored;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.Statistics;

import java.util.Arrays;

class BookmarkInfoShareable<T extends ShareableInfoProvider> extends BaseShareable
{
  @NonNull
  private final T mProvider;

  BookmarkInfoShareable(@NonNull Activity activity, @NonNull T provider,
                        @Nullable Sponsored sponsored)
  {
    super(activity);
    mProvider = provider;
    setSubject(R.string.bookmark_share_email_subject);

    String text = makeEmailBody(activity, sponsored, getEmailBodyContent());
    setText(text);
  }

  @NonNull
  private static String makeEmailBody(@NonNull Activity activity, @Nullable Sponsored sponsored,
                                      @NonNull Iterable<String> emailBodyContent)
  {
    String text = TextUtils.join(UiUtils.NEW_STRING_DELIMITER, emailBodyContent);

    if (sponsored != null && sponsored.getType() == Sponsored.TYPE_BOOKING)
      text = concatSponsoredText(activity, sponsored, text);

    return text;
  }

  @NonNull
  private static String concatSponsoredText(@NonNull Activity activity, @NonNull Sponsored sponsored,
                                            @NonNull String src)
  {
    String mainSegment = TextUtils.join(UiUtils.NEW_STRING_DELIMITER,
                                        Arrays.asList(src, activity.getString(R.string.sharing_booking)));
    return mainSegment + sponsored.getUrl();
  }

  @NonNull
  protected Iterable<String> getEmailBodyContent()
  {
    return TextUtils.isEmpty(getProvider().getAddress())
           ? Arrays.asList(getProvider().getName(), getGeoUrl(), getHttpUrl())
           : Arrays.asList(getProvider().getName(), getProvider().getAddress(), getGeoUrl(), getHttpUrl());
  }

  @NonNull
  protected T getProvider()
  {
    return mProvider;
  }

  @NonNull
  protected final String getGeoUrl()
  {
    return Framework.nativeGetGe0Url(getProvider().getLat(), getProvider().getLon(),
                                     getProvider().getScale(), getProvider().getName());
  }

  @NonNull
  protected final String getHttpUrl()
  {
    return Framework.getHttpGe0Url(getProvider().getLat(), getProvider().getLon(),
                                   getProvider().getScale(), getProvider().getName());
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
