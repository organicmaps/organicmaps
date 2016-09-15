package com.mapswithme.maps.editor.data;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.IntRange;
import android.text.format.DateFormat;

import com.mapswithme.maps.MwmApplication;

import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Locale;

public class HoursMinutes implements Parcelable
{
  public final long hours;
  public final long minutes;

  private final boolean is24HourFormat = DateFormat.is24HourFormat(MwmApplication.get());

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
    final String time = String.format(Locale.US, "%02d:%02d", hours, minutes);

    if (is24HourFormat)
      return time;

    SimpleDateFormat fmt24 = new SimpleDateFormat("HH:mm");
    SimpleDateFormat fmt12 = new SimpleDateFormat("hh:mm a");
    try
    {
      return fmt12.format(fmt24.parse(time));
    }
    catch (ParseException e)
    {
      return time;
    }
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
