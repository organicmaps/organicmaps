package app.organicmaps.widget.placepage.sections;

import static org.junit.Assert.*;

import android.app.Activity;
import android.app.Application;
import android.graphics.Rect;
import android.os.Build;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;
import app.organicmaps.R;
import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowLooper;

@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, sdk = {23, 35}, qualifiers = "ru-w600dp-h1000dp-mdpi")
public class ProductsAccessibilityTest
{
  @Test
  public void hitRegionsAreAtLeast48AndDoNotOverlapInEitherDirection()
  {
    for (int direction : new int[] {View.LAYOUT_DIRECTION_LTR, View.LAYOUT_DIRECTION_RTL})
    {
      Activity activity = Robolectric.buildActivity(Activity.class).setup().get();
      activity.setTheme(R.style.MwmTheme);
      ViewGroup host = (ViewGroup) LayoutInflater.from(activity).inflate(R.layout.place_page_products_fragment, null);
      host.setLayoutDirection(direction);
      activity.setContentView(host, new ViewGroup.LayoutParams(600, ViewGroup.LayoutParams.WRAP_CONTENT));
      ShadowLooper.idleMainLooper();
      AtomicInteger clicks = new AtomicInteger();
      int[] ids = {R.id.products_close, R.id.products_remind_later, R.id.products_already_donated};
      for (int id : ids)
        host.findViewById(id).setOnClickListener(v -> clicks.incrementAndGet());
      PlacePageProductsFragment.attachActionTargets(host);
      for (int scale : new int[] {1, 2})
      {
        ((TextView) host.findViewById(R.id.products_remind_later)).setTextSize(14 * scale);
        ((TextView) host.findViewById(R.id.products_already_donated)).setTextSize(14 * scale);
        ShadowLooper.idleMainLooper();
        host.measure(View.MeasureSpec.makeMeasureSpec(600, View.MeasureSpec.EXACTLY),
                     View.MeasureSpec.makeMeasureSpec(1000, View.MeasureSpec.AT_MOST));
        host.layout(0, 0, 600, host.getMeasuredHeight());
        for (int id : ids)
        {
          View target = host.findViewById(id);
          assertEquals(Button.class.getName(), target.createAccessibilityNodeInfo().getClassName().toString());
          assertTrue(target.isFocusable());
        }
        if (Build.VERSION.SDK_INT < 29)
          continue;
        var info = host.getTouchDelegate().getTouchDelegateInfo();
        assertEquals(3, info.getRegionCount());
        ArrayList<Rect> bounds = new ArrayList<>();
        for (int i = 0; i < info.getRegionCount(); ++i)
        {
          Rect rect = info.getRegionAt(i).getBounds();
          assertTrue(rect.toString(), rect.width() >= 48 && rect.height() >= 48);
          for (Rect old : bounds)
            assertFalse(Rect.intersects(old, rect));
          bounds.add(rect);
          int before = clicks.get();
          for (int action : new int[] {MotionEvent.ACTION_DOWN, MotionEvent.ACTION_UP})
          {
            MotionEvent event = MotionEvent.obtain(0, 0, action, rect.left + 1, rect.top + 1, 0);
            assertTrue("direction=" + direction + " scale=" + scale + " target=" + i + " action=" + action
                           + " bounds=" + rect + " host=" + host.getWidth() + "x" + host.getHeight(),
                       host.getTouchDelegate().onTouchEvent(event));
            event.recycle();
          }
          ShadowLooper.idleMainLooper();
          assertEquals(before + 1, clicks.get());
        }
      }
    }
  }
}
