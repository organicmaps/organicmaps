package com.mapswithme.maps.pins;

import com.mapswithme.maps.R;

import android.content.Context;
import android.content.Intent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.PopupWindow;

public class PinPopup extends PopupWindow
{

  private Button mButton;

  @SuppressWarnings("deprecation")
  public PinPopup(final Context context)
  {
    super(LayoutInflater.from(context).inflate(R.layout.pin_popup, null), LayoutParams.WRAP_CONTENT,
          LayoutParams.WRAP_CONTENT, true);
    mButton = (Button) getContentView().findViewById(R.id.popup_button);
    mButton.setOnClickListener(new OnClickListener()
    {

      @Override
      public void onClick(View v)
      {
        context.startActivity(new Intent(context, PinActivity.class).putExtra(PinActivity.PIN_ICON_ID,
                                                                              R.drawable.pin_red));
      }
    });
    setFocusable(false);
    setOutsideTouchable(false);
    getContentView().setOnTouchListener(new OnTouchListener()
    {

      @Override
      public boolean onTouch(View v, MotionEvent event)
      {
        return false;
      }
    });
    setAnimationStyle(R.style.PopupWindowAnimation);
  }

}
