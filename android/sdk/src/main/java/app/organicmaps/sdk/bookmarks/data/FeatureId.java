package app.organicmaps.sdk.bookmarks.data;

import android.os.Parcel;
import android.os.Parcelable;
import android.text.TextUtils;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import java.util.Objects;

/// @todo Review using of this class, because seems like it has no any useful purpose.
/// Just creating in JNI and assigning ..
public class FeatureId implements Parcelable
{
  @NonNull
  public static final FeatureId EMPTY = new FeatureId("", -1L, 0);

  @NonNull
  private final String mMwmName;
  private final long mMwmVersion;
  private final int mFeatureIndex;

  @NonNull
  public static FeatureId fromFeatureIdString(@NonNull String id)
  {
    if (TextUtils.isEmpty(id))
      throw new AssertionError("Feature id string is empty");

    String[] parts = id.split(":");
    if (parts.length != 3)
      throw new AssertionError("Wrong feature id string format");

    return new FeatureId(parts[1], Long.parseLong(parts[0]), Integer.parseInt(parts[2]));
  }

  // Used by JNI.
  @Keep
  private FeatureId(@NonNull String mwmName, long mwmVersion, int featureIndex)
  {
    mMwmName = mwmName;
    mMwmVersion = mwmVersion;
    mFeatureIndex = featureIndex;
  }

  private FeatureId(@NonNull Parcel in)
  {
    mMwmName = Objects.requireNonNull(in.readString());
    mMwmVersion = in.readLong();
    mFeatureIndex = in.readInt();
  }

  @Override
  public void writeToParcel(@NonNull Parcel dest, int flags)
  {
    dest.writeString(mMwmName);
    dest.writeLong(mMwmVersion);
    dest.writeInt(mFeatureIndex);
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @NonNull
  public String getMwmName()
  {
    return mMwmName;
  }

  public long getMwmVersion()
  {
    return mMwmVersion;
  }

  public int getFeatureIndex()
  {
    return mFeatureIndex;
  }

  @Override
  public boolean equals(Object o)
  {
    if (this == o)
      return true;
    if (o == null || getClass() != o.getClass())
      return false;

    FeatureId featureId = (FeatureId) o;

    if (mMwmVersion != featureId.mMwmVersion)
      return false;
    if (mFeatureIndex != featureId.mFeatureIndex)
      return false;
    return mMwmName.equals(featureId.mMwmName);
  }

  @Override
  public int hashCode()
  {
    return Objects.hash(mMwmName, mMwmVersion, mFeatureIndex);
  }

  @Override
  @NonNull
  public String toString()
  {
    return "FeatureId{"
  + "mMwmName='" + mMwmName + '\'' + ", mMwmVersion=" + mMwmVersion + ", mFeatureIndex=" + mFeatureIndex + '}';
  }

  public static final Creator<FeatureId> CREATOR = new Creator<>() {
    @Override
    @NonNull
    public FeatureId createFromParcel(@NonNull Parcel in)
    {
      return new FeatureId(in);
    }

    @Override
    @NonNull
    public FeatureId[] newArray(int size)
    {
      return new FeatureId[size];
    }
  };
}
