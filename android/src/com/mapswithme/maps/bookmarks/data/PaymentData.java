package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

public class PaymentData
{
  @NonNull
  private final String mServerId;
  @NonNull
  private final String mProductId;
  @NonNull
  private final String mName;
  @Nullable
  private final String mImgUrl;
  @NonNull
  private final String mAuthorName;

  public PaymentData(@NonNull String serverId, @NonNull String productId, @NonNull String name,
                     @Nullable String imgUrl, @NonNull String authorName)
  {
    mServerId = serverId;
    mProductId = productId;
    mName = name;
    mImgUrl = imgUrl;
    mAuthorName = authorName;
  }

  @NonNull
  public String getServerId()
  {
    return mServerId;
  }

  @NonNull
  public String getProductId()
  {
    return mProductId;
  }

  @NonNull
  public String getName()
  {
    return mName;
  }

  @Nullable
  public String getImgUrl()
  {
    return mImgUrl;
  }

  @NonNull
  public String getAuthorName()
  {
    return mAuthorName;
  }
}
