package com.mapswithme.maps.gallery;

import android.os.Parcel;
import android.os.Parcelable;

public class Image implements Parcelable {
  private final String url;

  public Image(String url) {
    this.url = url;
  }

  protected Image(Parcel in) {
    url = in.readString();
  }

  @Override
  public void writeToParcel(Parcel dest, int flags) {
    dest.writeString(url);
  }

  @Override
  public int describeContents() {
    return 0;
  }

  public static final Creator<Image> CREATOR = new Creator<Image>() {
    @Override
    public Image createFromParcel(Parcel in) {
      return new Image(in);
    }

    @Override
    public Image[] newArray(int size) {
      return new Image[size];
    }
  };

  public String getUrl() {
    return url;
  }
}
