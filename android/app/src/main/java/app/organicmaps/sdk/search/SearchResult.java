package app.organicmaps.sdk.search;

import android.content.Context;
import android.graphics.Typeface;
import android.os.Parcel;
import android.os.Parcelable;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.StyleSpan;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

import app.organicmaps.bookmarks.data.FeatureId;
import app.organicmaps.util.Distance;

/**
 * Class instances are created from native code.
 */
// Used by JNI.
@Keep
@SuppressWarnings("unused")
public class SearchResult implements Parcelable
{
  public static final int TYPE_PURE_SUGGEST = 0;
  public static final int TYPE_SUGGEST = 1;
  public static final int TYPE_RESULT = 2;

  // Values should match osm::YesNoUnknown enum.
  public static final int OPEN_NOW_UNKNOWN = 0;
  public static final int OPEN_NOW_YES = 1;
  public static final int OPEN_NOW_NO = 2;

  public static final SearchResult EMPTY = new SearchResult("", "", 0, 0,
      new int[]{}, new int[]{});

  // Used by JNI.
  @Keep
  @SuppressWarnings("unused")
  public static class Description implements Parcelable
  {
    public final FeatureId featureId;
    public final String localizedFeatureType;
    public final String region;
    public final Distance distance;

    public final String description;

    public final int openNow;
    public final int minutesUntilOpen;
    public final int minutesUntilClosed;
    public final boolean hasPopularityHigherPriority;

    public Description(FeatureId featureId, String featureType, String region, Distance distance,
                       String description, int openNow, int minutesUntilOpen, int minutesUntilClosed,
                       boolean hasPopularityHigherPriority)
    {
      this.featureId = featureId;
      this.localizedFeatureType = featureType;
      this.region = region;
      this.distance = distance;
      this.description = description;
      this.openNow = openNow;
      this.minutesUntilOpen = minutesUntilOpen;
      this.minutesUntilClosed = minutesUntilClosed;
      this.hasPopularityHigherPriority = hasPopularityHigherPriority;
    }

    public static final Creator<Description> CREATOR = new Creator<Description>()
    {
      @Override
      public Description createFromParcel(Parcel in)
      {
        return new Description(in);
      }

      @Override
      public Description[] newArray(int size)
      {
        return new Description[size];
      }
    };

    protected Description(Parcel in)
    {
      featureId = in.readParcelable(FeatureId.class.getClassLoader());
      localizedFeatureType = in.readString();
      region = in.readString();
      distance = in.readParcelable(Distance.class.getClassLoader());
      description = in.readString();
      openNow = in.readInt();
      minutesUntilOpen = in.readInt();
      minutesUntilClosed = in.readInt();
      hasPopularityHigherPriority = in.readByte() != 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags)
    {
      dest.writeParcelable(featureId, flags);
      dest.writeString(localizedFeatureType);
      dest.writeString(region);
      dest.writeParcelable(distance, flags);
      dest.writeString(description);
      dest.writeInt(openNow);
      dest.writeInt(minutesUntilOpen);
      dest.writeInt(minutesUntilClosed);
      dest.writeByte((byte) (hasPopularityHigherPriority ? 1 : 0));
    }

    @Override
    public int describeContents()
    {
      return 0;
    }
  }

  public final String name;
  public final String suggestion;
  public final double lat;
  public final double lon;

  public final int type;
  public final Description description;

  // Consecutive pairs of indexes (each pair contains : start index, length), specifying highlighted matches of original query in result
  public final int[] highlightRanges;
  public final int[] descHighlightRanges;

  @NonNull
  private final Popularity mPopularity;

  public SearchResult(String name, String suggestion, double lat, double lon, int[] highlightRanges, int[] descHighlightRanges)
  {
    this.name = name;
    this.suggestion = suggestion;
    this.lat = lat;
    this.lon = lon;
    this.description = null;
    // Looks like a hack, but it's fine. Otherwise, should make one more ctor and JNI code bloat.
    if (lat == 0 && lon == 0)
      this.type = TYPE_PURE_SUGGEST;
    else
      this.type = TYPE_SUGGEST;
    this.highlightRanges = highlightRanges;
    this.descHighlightRanges = descHighlightRanges;
    mPopularity = Popularity.defaultInstance();
  }

  public SearchResult(String name, Description description, double lat, double lon, int[] highlightRanges,
                      int[] descHighlightRanges, @NonNull Popularity popularity)
  {
    this.type = TYPE_RESULT;
    this.name = name;
    mPopularity = popularity;
    this.suggestion = null;
    this.lat = lat;
    this.lon = lon;
    this.description = description;
    this.highlightRanges = highlightRanges;
    this.descHighlightRanges = descHighlightRanges;
  }

  protected SearchResult(Parcel in)
  {
    name = in.readString();
    suggestion = in.readString();
    lat = in.readDouble();
    lon = in.readDouble();
    type = in.readInt();
    description = in.readParcelable(Description.class.getClassLoader());
    highlightRanges = in.createIntArray();
    descHighlightRanges = in.createIntArray();
    mPopularity = in.readParcelable(Popularity.class.getClassLoader());
  }

  @NonNull
  public String getTitle(@NonNull Context context)
  {
    String title = name;
    if (TextUtils.isEmpty(title) && description != null)
      title = description.localizedFeatureType;
    return title;
  }

  public void formatText(SpannableStringBuilder builder, int[] ranges)
  {
    if (ranges != null)
    {
      final int size = ranges.length / 2;
      int index = 0;
      for (int i = 0; i < size; i++)
      {
        final int start = ranges[index++];
        final int len = ranges[index++];

        builder.setSpan(new StyleSpan(Typeface.BOLD), start, start + len, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
      }
    }
  }

  @NonNull
  public Spannable getFormattedTitle(@NonNull Context context)
  {
    final String title = getTitle(context);
    final SpannableStringBuilder builder = new SpannableStringBuilder(title);
    formatText(builder, highlightRanges);

    return builder;
  }

  public Spannable getFormattedAddress(@NonNull Context context)
  {
    final String address = description != null ? description.region : null;
    final SpannableStringBuilder builder = new SpannableStringBuilder(address);
    formatText(builder, descHighlightRanges);

    return builder;
  }

  public static final Creator<SearchResult> CREATOR = new Creator<>()
  {
    @Override
    public SearchResult createFromParcel(Parcel in)
    {
      return new SearchResult(in);
    }

    @Override
    public SearchResult[] newArray(int size)
    {
      return new SearchResult[size];
    }
  };

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeString(name);
    dest.writeString(suggestion);
    dest.writeDouble(lat);
    dest.writeDouble(lon);
    dest.writeInt(type);
    dest.writeParcelable(description, flags);
    dest.writeIntArray(highlightRanges);
    dest.writeIntArray(descHighlightRanges);
    dest.writeParcelable(mPopularity, flags);
  }
}