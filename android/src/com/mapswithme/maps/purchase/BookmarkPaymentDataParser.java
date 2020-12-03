package com.mapswithme.maps.purchase;

import android.net.Uri;
import android.text.TextUtils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.bookmarks.data.PaymentData;
import com.mapswithme.util.CrashlyticsUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

public class BookmarkPaymentDataParser implements PaymentDataParser
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private static final String TAG = BookmarkPaymentDataParser.class.getSimpleName();

  public final static String SERVER_ID = "id";
  public final static String PRODUCT_ID = "tier";
  public final static String NAME = "name";
  public final static String IMG_URL = "img";
  public final static String AUTHOR_NAME = "author_name";

  @NonNull
  @Override
  public PaymentData parse(@NonNull String url)
  {
    Uri uri = Uri.parse(url);
    String serverId = getQueryRequiredParameter(uri, SERVER_ID);
    String productId = getQueryRequiredParameter(uri, PRODUCT_ID);
    String name = getQueryRequiredParameter(uri, NAME);
    String authorName = getQueryRequiredParameter(uri, AUTHOR_NAME);
    String group = PurchaseUtils.getTargetBookmarkGroupFromUri(uri);
    LOGGER.i(TAG, "Found target group: " + group);
    String imgUrl = uri.getQueryParameter(IMG_URL);
    return new PaymentData(serverId, productId, name, imgUrl, authorName, group);
  }

  @Nullable
  @Override
  public String getParameterByName(@NonNull String url, @NonNull String name)
  {
    Uri uri = Uri.parse(url);
    return uri.getQueryParameter(name);
  }

  @NonNull
  private static String getQueryRequiredParameter(@NonNull Uri uri, @NonNull String name)
  {
    String parameter = uri.getQueryParameter(name);
    if (TextUtils.isEmpty(parameter))
    {
      CrashlyticsUtils.INSTANCE.logException(
        new IllegalArgumentException("'" + name + "' parameter is required! URI: " + uri));
      return "";
    }

    return parameter;
  }
}
