package com.mapswithme.maps.editor.data;

public class UserStats
{
  public final int editsCount;
  public final int editorRank;
  public final String levelUp;
  public final long uploadTimestampSeconds;

  public UserStats(int editsCount, int editorRank, String levelUp, long seconds)
  {
    this.editsCount = editsCount;
    this.editorRank = editorRank;
    this.levelUp = levelUp;
    this.uploadTimestampSeconds = seconds;
  }
}
