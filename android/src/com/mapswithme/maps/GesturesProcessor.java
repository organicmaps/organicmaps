package com.mapswithme.maps;

import android.graphics.PointF;
import android.view.MotionEvent;

public class GesturesProcessor
{
  private final int NONE = 0;
  private final int MOVE = 1;
  private final int ZOOM = 2;

  // Do not modify this constant values (or take into account native code,
  // please).
  private final int START = 0;
  private final int PROCESS = 1;
  private final int END = 2;

  private PointF m_pt1;
  private PointF m_pt2;
  private int m_mode;

  private void getPointsMove(MotionEvent e)
  {
    m_pt1.set(e.getX(), e.getY());
  }

  private void getPointsZoom(MotionEvent e)
  {
    m_pt1.set(e.getX(0), e.getY(0));
    m_pt2.set(e.getX(1), e.getY(1));
  }

  public GesturesProcessor()
  {
    m_pt1 = new PointF();
    m_pt2 = new PointF();
    m_mode = NONE;
  }

  public void onTouchEvent(MotionEvent e)
  {
    switch (e.getAction() & MotionEvent.ACTION_MASK)
    {
    case MotionEvent.ACTION_DOWN:
      getPointsMove(e);
      m_mode = MOVE;
      nativeMove(START, m_pt1.x, m_pt1.y);
      break;

    case MotionEvent.ACTION_POINTER_DOWN:
      getPointsZoom(e);
      nativeZoom(START, m_pt1.x, m_pt1.y, m_pt2.x, m_pt2.y);
      break;

    case MotionEvent.ACTION_UP:
      getPointsMove(e);
      nativeMove(END, m_pt1.x, m_pt1.y);
      m_mode = NONE;
      break;

    case MotionEvent.ACTION_POINTER_UP:
      getPointsZoom(e);
      nativeZoom(END, m_pt1.x, m_pt1.y, m_pt2.x, m_pt2.y);
      m_mode = NONE;
      break;

    case MotionEvent.ACTION_MOVE:
      if (m_mode == MOVE)
      {
        getPointsMove(e);
        nativeMove(PROCESS, m_pt1.x, m_pt1.y);
      } else if (m_mode == ZOOM)
      {
        getPointsZoom(e);
        nativeZoom(PROCESS, m_pt1.x, m_pt1.y, m_pt2.x, m_pt2.y);
      }
      break;
    }
  }

  private native void nativeMove(int mode, double x, double y);

  private native void nativeZoom(int mode, double x1, double y1, double x2,
      double y2);
}
