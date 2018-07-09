package com.mapswithme.util.sharing;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.widget.placepage.Sponsored;
import com.mapswithme.util.statistics.Statistics;

class MapObjectShareable extends BaseShareable
{
  MapObjectShareable(Activity context, @NonNull MapObject mapObject, @Nullable Sponsored sponsored)
  {
    super(context);

    final Activity activity = getActivity();
    final String ge0Url = Framework.nativeGetGe0Url(mapObject.getLat(), mapObject.getLon(),
                                                    mapObject.getScale(), mapObject.getTitle());
    final String httpUrl = Framework.getHttpGe0Url(mapObject.getLat(), mapObject.getLon(),
                                                   mapObject.getScale(), mapObject.getTitle());
    final String subject;
    String text;
    if (MapObject.isOfType(MapObject.MY_POSITION, mapObject))
    {
      subject = activity.getString(R.string.my_position_share_email_subject);
      text = activity.getString(R.string.my_position_share_email,
                                Framework.nativeGetNameAndAddress(mapObject.getLat(), mapObject.getLon()),
                                ge0Url, httpUrl);
    }
    else
    {
      subject = activity.getString(R.string.bookmark_share_email_subject);

      text = lineWithBreak(activity.getString(R.string.sharing_call_action_look)) +
                 lineWithBreak(mapObject.getTitle()) +
                 lineWithBreak(mapObject.getSubtitle()) +
                 lineWithBreak(mapObject.getAddress()) +
                 lineWithBreak(ge0Url) +
                 lineWithBreak(httpUrl);

      if (sponsored != null && sponsored.getType() == Sponsored.TYPE_BOOKING)
      {
        text += lineWithBreak(activity.getString(R.string.sharing_booking)) +
                sponsored.getUrl();
      }
    }

    setSubject(subject);
    setText(text);
  }

  private String lineWithBreak(String title)
  {
    if (!TextUtils.isEmpty(title))
      return title + "\n";

    return "";
  }

  @Override
  public void share(SharingTarget target)
  {
    super.share(target);
    Statistics.INSTANCE.trackPlaceShared(target.name);
  }

  @Override
  public String getMimeType()
  {
    return null;
  }
}
