package com.mapswithme.yopme.bs;

import com.mapswithme.yopme.util.Utils;
import com.yotadevices.sdk.BSDrawer;
import com.yotadevices.sdk.BSMotionEvent;

public interface Backscreen
{
  public void onCreate();

  public void onResume();

  public void setUpView();

  public void onMotionEvent(BSMotionEvent motionEvent);

  public void draw(BSDrawer drawer);

  public void invalidate();

  public class Stub
  {
    public static Backscreen get()
    {
      return Utils.createInvocationLogger(Backscreen.class, "STUB_BACKVIEW");
    }
  }

}
