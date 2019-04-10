package com.mapswithme.maps.bookmarks;

import android.net.Uri;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.util.Log;

import com.mapswithme.maps.bookmarks.data.PaymentData;
import com.mapswithme.util.CrashlyticsUtils;

class BookmarkPaymentDataParser implements PaymentDataParser
{
  private static final String TAG = BookmarkPaymentDataParser.class.getSimpleName();

  final static String SERVER_ID = "id";
  final static String PRODUCT_ID = "tier";
  final static String NAME = "name";
  final static String IMG_URL = "img";
  final static String AUTHOR_NAME = "author_name";

  @NonNull
  @Override
  public PaymentData parse(@NonNull String url)
  {
    Uri uri = Uri.parse(url);
    String serverId = getQueryRequiredParameter(uri, SERVER_ID);
    String productId = getQueryRequiredParameter(uri, PRODUCT_ID);
    String name = getQueryRequiredParameter(uri, NAME);
    String authorName = getQueryRequiredParameter(uri, AUTHOR_NAME);
    String imgUrl = uri.getQueryParameter(IMG_URL);
    return new PaymentData(serverId, productId, name, imgUrl, authorName);
  }

  @Nullable
  String getParameter(@NonNull String url, @NonNull String parameterName)
  {
    Uri uri = Uri.parse(url);
    return uri.getQueryParameter(parameterName);
  }

  @NonNull
  private static String getQueryRequiredParameter(@NonNull Uri uri, @NonNull String name)
  {
    String parameter = uri.getQueryParameter(name);
    if (TextUtils.isEmpty(parameter))
    {
      CrashlyticsUtils.logException(
        new IllegalArgumentException("'" + name + "' parameter is required! URI: " + uri));
      return "";
    }

    return parameter;
  }
}
