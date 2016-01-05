package com.mapswithme.maps.editor.data;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.IntRange;

public class HoursMinutes implements Parcelable
{
  public final long hours;
  public final long minutes;

  public HoursMinutes(@IntRange(from = 0, to = 23) long hours, @IntRange(from = 0, to = 59) long minutes)
  {
    this.hours = hours;
    this.minutes = minutes;
  }

  protected HoursMinutes(Parcel in)
  {
    hours = in.readLong();
    minutes = in.readLong();
  }

  @Override
  public String toString()
  {
    return hours + "." + minutes;
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeLong(hours);
    dest.writeLong(minutes);
  }

  public static final Creator<HoursMinutes> CREATOR = new Creator<HoursMinutes>()
  {
    @Override
    public HoursMinutes createFromParcel(Parcel in)
    {
      return new HoursMinutes(in);
    }

    @Override
    public HoursMinutes[] newArray(int size)
    {
      return new HoursMinutes[size];
    }
  };
}
