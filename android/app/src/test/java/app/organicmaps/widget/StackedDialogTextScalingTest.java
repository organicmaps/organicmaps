package app.organicmaps.widget;

import static org.junit.Assert.*;

import android.app.Activity;
import android.app.Application;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ScrollView;
import android.widget.TextView;
import app.organicmaps.R;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.GraphicsMode;

@RunWith(RobolectricTestRunner.class)
@GraphicsMode(GraphicsMode.Mode.NATIVE)
@Config(application = Application.class, sdk = 35, qualifiers = "ru-mdpi")
public class StackedDialogTextScalingTest
{
  @Test
  public void largeLabelsGrowAndAllChoicesCanBeScrolledIntoView()
  {
    Activity activity = Robolectric.buildActivity(Activity.class).setup().get();
    activity.setTheme(R.style.MwmTheme);
    ScrollView root = (ScrollView) LayoutInflater.from(activity).inflate(R.layout.dialog_stacked_buttons, null);
    activity.setContentView(root);
    int[] ids = {R.id.btn__positive, R.id.btn__neutral, R.id.btn__negative};
    for (int id : ids)
    {
      TextView button = root.findViewById(id);
      button.setText("Использовать мобильный интернет только сегодня или при следующем запуске");
      button.setTextSize(android.util.TypedValue.COMPLEX_UNIT_PX, button.getTextSize() * 2);
    }
    root.measure(View.MeasureSpec.makeMeasureSpec(280, View.MeasureSpec.EXACTLY),
                 View.MeasureSpec.makeMeasureSpec(220, View.MeasureSpec.EXACTLY));
    root.layout(0, 0, 280, 220);
    for (int id : ids)
    {
      TextView button = root.findViewById(id);
      assertTrue(button.getHeight() >= 48);
      assertTrue(button.getLineCount() > 2);
      int available = button.getHeight() - button.getCompoundPaddingTop() - button.getCompoundPaddingBottom();
      assertTrue(button.getLayout().getHeight() <= available);
    }
    assertTrue(root.getChildAt(0).getHeight() > root.getHeight());
    assertTrue(root.fullScroll(View.FOCUS_DOWN));
  }
}
