package com.mapswithme.maps.bookmarks.data;

import android.os.Parcel;
import android.os.Parcelable;
import androidx.annotation.Nullable;

public class Result implements Parcelable
{
  @Nullable
  private final String mFilePath;
  @Nullable
  private final String mArchiveId;

  public Result(@Nullable String filePath, @Nullable String archiveId)
  {
    mFilePath = filePath;
    mArchiveId = archiveId;
  }

  protected Result(Parcel in)
  {
    mFilePath = in.readString();
    mArchiveId = in.readString();
  }

  @Nullable
  public String getFilePath()
  {
    return mFilePath;
  }

  @Nullable
  public String getArchiveId()
  {
    return mArchiveId;
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeString(mFilePath);
    dest.writeString(mArchiveId);
  }

  @Override
  public String toString()
  {
    return "Result{" +
           "mFilePath='" + mFilePath + '\'' +
           ", mArchiveId='" + mArchiveId + '\'' +
           '}';
  }

  public static final Creator<Result> CREATOR = new Creator<Result>()
  {
    @Override
    public Result createFromParcel(Parcel in)
    {
      return new Result(in);
    }

    @Override
    public Result[] newArray(int size)
    {
      return new Result[size];
    }
  };
}
