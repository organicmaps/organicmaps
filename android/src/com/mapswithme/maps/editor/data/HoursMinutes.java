package com.mapswithme.maps.editor.data;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.IntRange;
import android.text.format.DateFormat;

import com.mapswithme.maps.MwmApplication;

import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.GregorianCalendar;
import java.util.Locale;

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

  private boolean is24HourFormat() { return DateFormat.is24HourFormat(MwmApplication.get()); }

  @Override
  public String toString()
  {
    if (is24HourFormat())
      return String.format(Locale.US, "%02d:%02d", hours, minutes);

    Calendar calendar = new GregorianCalendar();
    calendar.set(Calendar.HOUR_OF_DAY, (int)hours);
    calendar.set(Calendar.MINUTE, (int)minutes);

    SimpleDateFormat fmt12 = new SimpleDateFormat("hh:mm a");

    return fmt12.format(calendar.getTime());
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
