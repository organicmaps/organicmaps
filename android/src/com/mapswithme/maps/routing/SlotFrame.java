package com.mapswithme.maps.routing;

import android.support.annotation.IdRes;
import android.view.DragEvent;
import android.view.View;
import android.widget.TextView;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;

class SlotFrame implements View.OnDragListener
{
  private final View mFrame;
  private final Slot mSlotFrom;
  private final Slot mSlotTo;
  private Slot mDraggedSlot;

  private static class DragData
  {
    final MapObject mapObject;
    final int order;

    private DragData(MapObject mapObject, int order)
    {
      this.mapObject = mapObject;
      this.order = order;
    }
  }

  private static class Slot
  {
    private final View mFrame;
    private final TextView mOrderText;
    private final TextView mText;
    private final View mDragHandle;

    private int mOrder;
    private MapObject mMapObject;

    Slot(View frame, @IdRes int id, int order)
    {
      mFrame = frame.findViewById(id);
      mOrderText = (TextView) mFrame.findViewById(R.id.order);
      mText = (TextView) mFrame.findViewById(R.id.text);
      mDragHandle = mFrame.findViewById(R.id.drag_handle);

      setOrger(order);
      setMapObject(null);
    }

    void setOrger(int order)
    {
      mOrder = order;
      mOrderText.setText(String.valueOf(order));
      if (order == 1)
        mText.setHint(R.string.choose_starting_point);
      else
        mText.setHint(R.string.choose_destination);
    }

    boolean onDrag(View view, DragEvent event)
    {
      return false;
    }

    void setMapObject(MapObject mapObject)
    {
      mMapObject = mapObject;
      if (mMapObject != null)
        mMapObject.setDefaultIfEmpty();

      mText.setText((mMapObject == null) ? "" : mMapObject.getName());
    }

    void swap(Slot other)
    {
      int order = mOrder;
      setOrger(other.mOrder);
      other.setOrger(order);

      MapObject mapObject = mMapObject;
      setMapObject(other.mMapObject);
      other.setMapObject(mapObject);
    }
  }

  public SlotFrame(View root)
  {
    mFrame = root.findViewById(R.id.slots);

    mSlotFrom = new Slot(mFrame, R.id.from, 1);
    mSlotTo = new Slot(mFrame, R.id.to, 2);

    mFrame.setOnDragListener(this);
  }

  public void update()
  {
    mSlotFrom.setMapObject(RoutingController.get().getStartPoint());
    mSlotTo.setMapObject(RoutingController.get().getEndPoint());
  }

  public View getFrame()
  {
    return mFrame;
  }

  @Override
  public boolean onDrag(View view, DragEvent event)
  {
    if (mDraggedSlot != null)
      return mDraggedSlot.onDrag(view, event);

    if (mSlotFrom.onDrag(view, event))
    {
      mDraggedSlot = mSlotFrom;
      return true;
    }

    if (mSlotTo.onDrag(view, event))
    {
      mDraggedSlot = mSlotTo;
      return true;
    }

    return false;
  }
}
