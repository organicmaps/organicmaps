package com.mapswithme.yopme;

import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.app.Activity;
import android.graphics.Color;

public class ReferenceActivity extends Activity
{

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.help);
    final ViewGroup rootView = (ViewGroup) findViewById(R.id.root);

    for (int i = 0; i < rootView.getChildCount(); ++i)
    {
      final View childView = rootView.getChildAt(i);
      {
        if (childView instanceof TextView)
          ((TextView)childView).setTextColor(Color.BLACK);
      }
    }
  }

}
