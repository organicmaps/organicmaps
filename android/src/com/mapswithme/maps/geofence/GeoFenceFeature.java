package com.mapswithme.maps.geofence;

import android.os.Parcel;
import android.os.Parcelable;
import androidx.annotation.NonNull;

import com.mapswithme.maps.bookmarks.data.FeatureId;

/**
 * Represents CampaignFeature from core.
 */
public class GeoFenceFeature implements Parcelable
{
  @NonNull
  private final FeatureId mFeatureId;
  private final double latitude;
  private final double longitude;

  public GeoFenceFeature(@NonNull FeatureId featureId, double latitude, double longitude)
  {
    mFeatureId = featureId;
    this.latitude = latitude;
    this.longitude = longitude;
  }

  public double getLatitude()
  {
    return latitude;
  }

  public double getLongitude()
  {
    return longitude;
  }

  @NonNull
  public FeatureId getId()
  {
    return mFeatureId;
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeParcelable(this.mFeatureId, flags);
    dest.writeDouble(this.latitude);
    dest.writeDouble(this.longitude);
  }

  protected GeoFenceFeature(Parcel in)
  {
    this.mFeatureId = in.readParcelable(FeatureId.class.getClassLoader());
    this.latitude = in.readDouble();
    this.longitude = in.readDouble();
  }

  public static final Creator<GeoFenceFeature> CREATOR = new Creator<GeoFenceFeature>()
  {
    @Override
    public GeoFenceFeature createFromParcel(Parcel source)
    {
      return new GeoFenceFeature(source);
    }

    @Override
    public GeoFenceFeature[] newArray(int size)
    {
      return new GeoFenceFeature[size];
    }
  };

  @Override
  public String toString()
  {
    final StringBuilder sb = new StringBuilder("GeoFenceFeature{");
    sb.append("mFeatureId=").append(mFeatureId);
    sb.append(", latitude=").append(latitude);
    sb.append(", longitude=").append(longitude);
    sb.append('}');
    return sb.toString();
  }

  @Override
  public boolean equals(Object o)
  {
    if (this == o) return true;
    if (o == null || getClass() != o.getClass()) return false;

    GeoFenceFeature that = (GeoFenceFeature) o;

    return mFeatureId.equals(that.mFeatureId);
  }

  @Override
  public int hashCode()
  {
    return mFeatureId.hashCode();
  }
}
