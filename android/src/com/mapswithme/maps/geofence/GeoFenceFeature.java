package com.mapswithme.maps.geofence;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.NonNull;

/**
 * Represents CampaignFeature from core.
 */
public class GeoFenceFeature implements Parcelable
{
  private final long mwmVersion;
  @NonNull
  private final String countryId;
  private final int featureIndex;
  private final double latitude;
  private final double longitude;

  public GeoFenceFeature(long mwmVersion, @NonNull String countryId, int featureIndex,
                         double latitude, double longitude)
  {
    this.mwmVersion = mwmVersion;
    this.countryId = countryId;
    this.featureIndex = featureIndex;
    this.latitude = latitude;
    this.longitude = longitude;
  }

  public long getMwmVersion()
  {
    return mwmVersion;
  }

  @NonNull
  public String getCountryId()
  {
    return countryId;
  }

  public int getFeatureIndex()
  {
    return featureIndex;
  }

  public double getLatitude()
  {
    return latitude;
  }

  public double getLongitude()
  {
    return longitude;
  }

  @Override
  public boolean equals(Object o)
  {
    if (this == o) return true;
    if (o == null || getClass() != o.getClass()) return false;

    GeoFenceFeature that = (GeoFenceFeature) o;

    if (mwmVersion != that.mwmVersion) return false;
    if (featureIndex != that.featureIndex) return false;
    return countryId.equals(that.countryId);
  }

  @Override
  public int hashCode()
  {
    int result = (int) (mwmVersion ^ (mwmVersion >>> 32));
    result = 31 * result + countryId.hashCode();
    result = 31 * result + featureIndex;
    return result;
  }

  @NonNull
  public String getId()
  {
    return String.valueOf(hashCode());
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeLong(this.mwmVersion);
    dest.writeString(this.countryId);
    dest.writeInt(this.featureIndex);
    dest.writeDouble(this.latitude);
    dest.writeDouble(this.longitude);
  }

  protected GeoFenceFeature(Parcel in)
  {
    this.mwmVersion = in.readLong();
    this.countryId = in.readString();
    this.featureIndex = in.readInt();
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
}
