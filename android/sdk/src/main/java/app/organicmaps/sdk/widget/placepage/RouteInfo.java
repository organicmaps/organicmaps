package app.organicmaps.sdk.widget.placepage;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

public final class RouteInfo
{
  @NonNull
  private final String mRef;
  @NonNull
  private final String mFrom;
  @NonNull
  private final String mTo;
  private final int mType;
  private final int mRelId;
  /// ARGB; 0 (fully transparent) means the relation has no color tag.
  private final int mColor;

  // Used by JNI.
  @Keep
  public RouteInfo(@NonNull String ref, @NonNull String from, @NonNull String to, int type, int relId, int color)
  {
    mRef = ref;
    mFrom = from;
    mTo = to;
    mType = type;
    mRelId = relId;
    mColor = color;
  }

  @NonNull
  public String getRef()
  {
    return mRef;
  }

  @NonNull
  public String getFrom()
  {
    return mFrom;
  }

  @NonNull
  public String getTo()
  {
    return mTo;
  }

  public int getType()
  {
    return mType;
  }

  public int getRelId()
  {
    return mRelId;
  }

  public int getColor()
  {
    return mColor;
  }

  public boolean hasColor()
  {
    return (mColor >>> 24) != 0;
  }

  /// User-facing single-line label: "ref" or "ref: from → to".
  @NonNull
  public String formatLabel()
  {
    if (mFrom.isEmpty() && mTo.isEmpty())
      return mRef;
    final StringBuilder sb = new StringBuilder(mRef).append(": ").append(mFrom);
    if (!mTo.isEmpty())
      sb.append(" → ").append(mTo);
    return sb.toString();
  }
}
