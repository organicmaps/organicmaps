package com.mapswithme.util;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;

import com.facebook.AppEventsLogger;
import com.facebook.FacebookRequestError;
import com.facebook.HttpMethod;
import com.facebook.Request;
import com.facebook.RequestAsyncTask;
import com.facebook.Response;
import com.facebook.Session;
import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.R;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.SimpleLogger;
import com.mapswithme.util.statistics.Statistics;

import java.util.Arrays;
import java.util.Calendar;
import java.util.Collection;
import java.util.List;


public class FbUtil
{

  public static final String PROMO_IMAGE_URL = "http://static.mapswithme.com/images/17th_august_promo.jpg";
  private static final String PROMO_MARKET_URL = "http://maps.me/get?17auga";

  public static Logger mLogger = SimpleLogger.get("MWM_FB");

  private static final List<String> PUBLISH_PERMISSIONS = Arrays.asList("publish_actions");

  public static void activate(Context context)
  {
    if (!Statistics.INSTANCE.isStatisticsEnabled(context))
      return;

    mLogger.d("ACTIVATING");
    AppEventsLogger.activateApp(context, context.getString(R.string.fb_app_id));
  }

  public static boolean isSubsetOf(Collection<String> subset, Collection<String> superset)
  {
    for (String string : subset)
    {
      if (!superset.contains(string))
        return false;
    }

    return true;
  }

  /**
   * Tries to get session with publish permissions and create a promo post.
   *
   * @param activity
   * @return true, if should reauthorize
   */
  public static boolean makeFbPromoPost(Activity activity)
  {
    Session session = Session.getActiveSession();

    if (session != null)
    {
      // check publish permissions
      List<String> permissions = session.getPermissions();
      if (!FbUtil.isSubsetOf(PUBLISH_PERMISSIONS, permissions))
      {
        Session.NewPermissionsRequest newPermissionsRequest = new Session
            .NewPermissionsRequest(activity, PUBLISH_PERMISSIONS);
        session.requestNewPublishPermissions(newPermissionsRequest);
        return true;
      }

      final Bundle postParams = new Bundle();
      postParams.putString("message", activity.getString(R.string.maps_me_is_free_today_facebook_post_android));
      postParams.putString("link", PROMO_MARKET_URL);
      postParams.putString("picture", PROMO_IMAGE_URL);

      Request.Callback callback = new Request.Callback()
      {
        public void onCompleted(Response response)
        {
          FacebookRequestError error = response.getError();
          if (error != null)
            Utils.toastShortcut(MWMApplication.get(), error.getErrorMessage());
        }
      };

      Request request = new Request(session, "me/feed", postParams, HttpMethod.POST, callback);

      RequestAsyncTask task = new RequestAsyncTask(request);
      task.execute();
    }

    return false;
  }

  public static boolean isPromoTimeNow()
  {
    final int promoYear = 2014;
    final int promoDateStart = 17;
    final int promoDateEnd = 18;
    final int promoHourStart = 2;
    final int promoHourEnd = 22;
    final Calendar calendar = Calendar.getInstance();
    calendar.set(Calendar.YEAR, promoYear);
    calendar.set(Calendar.MONTH, Calendar.AUGUST);
    calendar.set(Calendar.DAY_OF_MONTH, promoDateStart);
    calendar.set(Calendar.HOUR_OF_DAY, promoHourStart);
    calendar.set(Calendar.MINUTE, 0);
    calendar.set(Calendar.SECOND, 0);
    final long startMillis = calendar.getTimeInMillis();
    calendar.set(Calendar.DAY_OF_MONTH, promoDateEnd);
    calendar.set(Calendar.HOUR_OF_DAY, promoHourEnd);
    final long endMillis = calendar.getTimeInMillis();

    return System.currentTimeMillis() > startMillis && System.currentTimeMillis() < endMillis;
  }

  private FbUtil() {}
}
