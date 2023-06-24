package app.organicmaps.editor.data;

public class Timespan
{
  public final HoursMinutes start;
  public final HoursMinutes end;

  public Timespan(HoursMinutes start, HoursMinutes end)
  {
    this.start = start;
    this.end = end;
  }

  @Override
  public String toString()
  {
    return start + "-" + end;
  }

  public String toWideString()
  {
    return start + "\u2014" + end;
  }
}
