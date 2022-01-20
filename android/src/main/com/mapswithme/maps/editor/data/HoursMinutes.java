package com.mapswithme.maps.editor.data;

import android.os.Parcel;
import android.os.Parcelable;

import androidx.annotation.IntRange;

import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.GregorianCalendar;
import java.util.Locale;

public class HoursMinutes implements Parcelable
{
  public final long hours;
  public final long minutes;
  private final boolean m24HourFormat;

  public HoursMinutes(@IntRange(from = 0, to = 23) long hours,
                      @IntRange(from = 0, to = 59) long minutes, boolean is24HourFormat)
  {
    this.hours = hours;
    this.minutes = minutes;
    m24HourFormat = is24HourFormat;
  }

  protected HoursMinutes(Parcel in)
  {
    hours = in.readLong();
    minutes = in.readLong();
    m24HourFormat = in.readByte() != 0;
  }

  @Override
  public String toString()
  {
    if (m24HourFormat)
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
    dest.writeByte((byte) (m24HourFormat ? 1 : 0));
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
