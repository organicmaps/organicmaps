package com.mapswithme.maps.pins.pins;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

import com.mapswithme.maps.R;
import com.mapswithme.maps.R.drawable;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

public class PinManager
{
  private static PinManager sManager;
  private List<Pin> mPins;
  private List<PinSet> mPinSets;
  private Context mContext;
  private Map<Integer, Icon> mIcons;

  private PinManager(Context context)
  {
    mContext = context;
    refreshList();
  }

  public static PinManager getPinManager(Context context)
  {
    if (sManager == null)
    {
      sManager = new PinManager(context.getApplicationContext());
    }

    return sManager;
  }

  public List<PinSet> getPinSets()
  {

    return mPinSets;
  }

  public int getPinId(Pin pin)
  {
    return mPins.indexOf(pin);
  }

  public Pin createNewPin()
  {
    Pin p;
    mPins.add(p = new Pin("", mIcons.get(R.drawable.pin_red)));
    return p;
  }

  public Pin getPin(int index)
  {
    if (index >= 0 && index < mPins.size())
    {
      return mPins.get(index);
    } else
    {
      return null;
    }
  }

  public int getSetId(PinSet set)
  {
    return mPinSets.indexOf(set);
  }

  public PinSet getSet(int index)
  {
    if (index >= 0 && index < mPinSets.size())
    {
      return mPinSets.get(index);
    } else
    {
      return null;
    }
  }

  public List<Icon> getIcons()
  {
    return new ArrayList<Icon>(mIcons.values());
  }

  public Icon getIcon(int id)
  {
    return mIcons.get(id);
  }

  boolean savePin(Pin pin)
  {
    return false;
  }

  public void deleteSet(PinSet set)
  {
    mPins.removeAll(set.getPins());
    mPinSets.remove(set);
  }

  /**
   * Вызывать в onStart() (мало ли, может свернул, закинул закладку и вернулся)
   * , при создании менеджера.
   */
  private void refreshList()
  {
    mPins = new ArrayList<Pin>();
    mPinSets = new ArrayList<PinSet>();
    mIcons = loadIcons();
    for (int i = 0; i < 25; i++)
    {
      PinSet set = new PinSet("Set " + (i + 1));
      mPinSets.add(set);
      for (Icon icon : mIcons.values())
      {
        Pin p = new Pin(set.getName() + " pin #" + icon.getDrawableId(), icon);
        p.setPinSet(set);
        mPins.add(p);
      }
    }
  }

  private Map<Integer, Icon> loadIcons()
  {
    Map<Integer, Icon> icons = new HashMap<Integer, Icon>();
    R.drawable dr = new drawable();
    Class<? extends drawable> drawableClass = dr.getClass();
    Field[] fields = drawableClass.getFields();
    for (Field field : fields)
    {
      if (field.getName().startsWith("pin_"))
      {
        try
        {
          int id = field.getInt(drawableClass);
          icons.put(id,
              new Icon(field.getName().replace(".png", ""), BitmapFactory.decodeResource(mContext.getResources(), id),
                  id));
        } catch (IllegalArgumentException e)
        {
          // TODO Auto-generated catch block
          e.printStackTrace();
        } catch (IllegalAccessException e)
        {
          // TODO Auto-generated catch block
          e.printStackTrace();
        }
      }
    }
    return icons;
  }

  public void deletePin(Pin pin)
  {
    mPins.remove(pin);
    pin.getPinSet().removePin(pin);
  }
}
