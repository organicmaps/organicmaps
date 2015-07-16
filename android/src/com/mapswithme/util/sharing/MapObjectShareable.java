package com.mapswithme.util.sharing;

import android.app.Activity;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.util.statistics.Statistics;

public class MapObjectShareable extends BaseShareable
{
  protected final MapObject mMapObject;


  public MapObjectShareable(Activity context, MapObject mapObject)
  {
    super(context);
    mMapObject = mapObject;

    String ge0Url = Framework.nativeGetGe0Url(mMapObject.getLat(), mMapObject.getLon(), mMapObject.getScale(), mMapObject.getName());
    String httpUrl = Framework.getHttpGe0Url(mMapObject.getLat(), mMapObject.getLon(), mMapObject.getScale(), mMapObject.getName());
    String address = Framework.nativeGetNameAndAddress4Point(mMapObject.getLat(), mMapObject.getLon());
    int resId = mMapObject.getType() == MapObject.MapObjectType.MY_POSITION ? R.string.my_position_share_email
                                                                            : R.string.my_position_share_email_subject;
    setText(getActivity().getString(resId, address, ge0Url, httpUrl));
    setSubject(getActivity().getString(mMapObject.getType() == MapObject.MapObjectType.MY_POSITION ? R.string.my_position_share_email_subject
                                                                                                  : R.string.bookmark_share_email_subject));
  }

  @Override
  public void share(SharingTarget target)
  {
    super.share(target);
    Statistics.INSTANCE.trackPlaceShared(getClass().getSimpleName());
  }

  @Override
  public String getMimeType()
  {
    return null;
  }
}
