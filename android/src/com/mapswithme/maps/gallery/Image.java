package com.mapswithme.maps.gallery;

import android.os.Parcel;
import android.os.Parcelable;

public class Image implements Parcelable {
  private final String url;
  private final String smallUrl;
  private String description;
  private String userName;
  private String userAvatar;
  private String source;
  private Long date;

  @SuppressWarnings("unused")
  public Image(String url, String smallUrl) {
    this.url = url;
    this.smallUrl = smallUrl;
  }

  protected Image(Parcel in) {
    url = in.readString();
    smallUrl = in.readString();
    description = in.readString();
    userName = in.readString();
    userAvatar = in.readString();
    source = in.readString();
    date = (Long) in.readValue(Long.class.getClassLoader());
  }

  @Override
  public void writeToParcel(Parcel dest, int flags) {
    dest.writeString(url);
    dest.writeString(smallUrl);
    dest.writeString(description);
    dest.writeString(userName);
    dest.writeString(userAvatar);
    dest.writeString(source);
    dest.writeValue(date);
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

  public String getSmallUrl() {
    return smallUrl;
  }

  public String getDescription() {
    return description;
  }

  public void setDescription(String description) {
    this.description = description;
  }

  String getUserName() {
    return userName;
  }

  @SuppressWarnings("unused")
  public void setUserName(String userName) {
    this.userName = userName;
  }

  String getUserAvatar() {
    return userAvatar;
  }

  @SuppressWarnings("unused")
  public void setUserAvatar(String userAvatar) {
    this.userAvatar = userAvatar;
  }

  public String getSource() {
    return source;
  }

  public void setSource(String source) {
    this.source = source;
  }

  public Long getDate() {
    return date;
  }

  public void setDate(Long date) {
    this.date = date;
  }
}
