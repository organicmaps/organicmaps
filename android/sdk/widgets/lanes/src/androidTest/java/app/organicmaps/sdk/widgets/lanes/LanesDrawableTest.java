package app.organicmaps.sdk.widgets.lanes;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import androidx.test.platform.app.InstrumentationRegistry;
import app.organicmaps.sdk.routing.LaneInfo;
import app.organicmaps.sdk.routing.LaneWay;
import app.organicmaps.sdk.util.Graphics;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import org.junit.Assert;
import org.junit.Test;

public class LanesDrawableTest
{
  private static final int ACTIVE = Color.WHITE;
  private static final int INACTIVE = Color.rgb(119, 119, 119);

  @Test
  public void renderVilniusRightFromMwm() throws IOException
  {
    // Lithuania_Vilnius_LuksioKalvariju_LanesTest, issue #9429:
    // turn:lanes:backward=left;through|right, route turns right.
    final LaneInfo[] vilniusLanes = {
        new LaneInfo(new LaneWay[] {LaneWay.Left, LaneWay.Through}, LaneWay.None),
        new LaneInfo(new LaneWay[] {LaneWay.Right}, LaneWay.Right),
    };
    final Bitmap vilnius = render("vilnius_right", vilniusLanes);
    Assert.assertEquals(INACTIVE, vilnius.getPixel(32, 15));
    Assert.assertEquals(INACTIVE, vilnius.getPixel(32, 54));
    Assert.assertEquals(INACTIVE, vilnius.getPixel(15, 20));
    Assert.assertEquals(ACTIVE, vilnius.getPixel(64 + 40, 12));
    Assert.assertEquals(Color.BLACK, vilnius.getPixel(25, 54));
    renderTheme("vilnius_light", vilniusLanes, Color.rgb(36, 156, 242), Color.WHITE);
    renderTheme("vilnius_night", vilniusLanes, Color.rgb(75, 185, 230), Color.argb(222, 0, 0, 0));
  }

  @Test
  public void renderLjubljanaExitFromMwm() throws IOException
  {
    // Slovenia_Ljubljana_GolovecTunnelExit_LanesTest, issue #7845:
    // turn:lanes=slight_left|slight_left;slight_right|slight_right.
    final LaneInfo[] ljubljanaLanes = {
        new LaneInfo(new LaneWay[] {LaneWay.SlightLeft}, LaneWay.SlightLeft),
        new LaneInfo(new LaneWay[] {LaneWay.SlightLeft, LaneWay.SlightRight}, LaneWay.SlightLeft),
        new LaneInfo(new LaneWay[] {LaneWay.SlightRight}, LaneWay.None),
    };
    final Bitmap ljubljana = render("ljubljana_exit", ljubljanaLanes);
    Assert.assertEquals(ACTIVE, ljubljana.getPixel(64 + 9, 9));
    Assert.assertEquals(INACTIVE, ljubljana.getPixel(64 + 52, 13));
    Assert.assertEquals(ACTIVE, ljubljana.getPixel(64 + 32, 54));
    renderTheme("ljubljana_light", ljubljanaLanes, Color.rgb(36, 156, 242), Color.WHITE);
    renderTheme("ljubljana_night", ljubljanaLanes, Color.rgb(75, 185, 230), Color.argb(222, 0, 0, 0));

    final Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    final Bitmap carImage = Graphics.drawableToBitmap(new LanesDrawable(context, ljubljanaLanes));
    final float density = context.getResources().getDisplayMetrics().density;
    Assert.assertTrue(carImage.getWidth() <= 500 * density);
    Assert.assertTrue(carImage.getHeight() <= 74 * density);
    save(context, "lane_glyph_ljubljana_car_image.png", carImage);
  }

  @Test
  public void renderLesnoyOsmLanes() throws IOException
  {
    // data/test_data/osm/lesnoy_town.osm, way 475694057:
    // turn:lanes:forward=reverse;left|left;through;right.
    final Bitmap reverseLeft =
        render("lesnoy_left",
               new LaneInfo[] {
                   new LaneInfo(new LaneWay[] {LaneWay.ReverseLeft, LaneWay.ReverseRight, LaneWay.Left}, LaneWay.Left),
                   new LaneInfo(new LaneWay[] {LaneWay.Left, LaneWay.Through, LaneWay.Right}, LaneWay.Left),
               });
    Assert.assertEquals(INACTIVE, reverseLeft.getPixel(19, 50));
    Assert.assertEquals(Color.BLACK, reverseLeft.getPixel(58, 42));
    Assert.assertEquals(ACTIVE, reverseLeft.getPixel(22, 25));
  }

  @Test
  public void renderMoscowMwmLanesAndCarImage() throws IOException
  {
    // Russia_Moscow_ZemlyanoyVal_LanesTest: the route turns right and the
    // sixth lane still has an unselected, side-ambiguous OSM "reverse" tag.
    final LaneInfo[] moscowLanes = {
        new LaneInfo(new LaneWay[] {LaneWay.Through}, LaneWay.None),
        new LaneInfo(new LaneWay[] {LaneWay.Through}, LaneWay.None),
        new LaneInfo(new LaneWay[] {LaneWay.Through}, LaneWay.None),
        new LaneInfo(new LaneWay[] {LaneWay.Through}, LaneWay.None),
        new LaneInfo(new LaneWay[] {LaneWay.Right}, LaneWay.Right),
        new LaneInfo(new LaneWay[] {LaneWay.ReverseLeft, LaneWay.ReverseRight}, LaneWay.None),
    };
    final Bitmap moscow = render("moscow_right", moscowLanes);
    Assert.assertEquals(INACTIVE, moscow.getPixel(5 * 64 + 12, 42));
    Assert.assertEquals(Color.BLACK, moscow.getPixel(5 * 64 + 58, 42));

    // RoutingUtils supplies this same bitmap to Step.Builder.setLanesImage().
    final Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    final Bitmap carImage = Graphics.drawableToBitmap(new LanesDrawable(context, moscowLanes));
    final float density = context.getResources().getDisplayMetrics().density;
    Assert.assertTrue(carImage.getWidth() <= 500 * density);
    Assert.assertTrue(carImage.getHeight() <= 74 * density);
    save(context, "lane_glyph_moscow_car_image.png", carImage);
  }

  @Test
  public void renderRightSideReverse() throws IOException
  {
    final Bitmap reverseRight = render(
        "reverse_right", new LaneInfo[] {
                             new LaneInfo(new LaneWay[] {LaneWay.ReverseRight, LaneWay.Right}, LaneWay.ReverseRight),
                         });
    Assert.assertEquals(ACTIVE, reverseRight.getPixel(45, 50));
    Assert.assertEquals(Color.BLACK, reverseRight.getPixel(6, 42));
    Assert.assertEquals(INACTIVE, reverseRight.getPixel(56, 18));
  }

  @Test
  public void renderReverseWithSameSideTurns() throws IOException
  {
    final LaneInfo[] leftLanes = {
        new LaneInfo(new LaneWay[] {LaneWay.ReverseLeft, LaneWay.SharpLeft}, LaneWay.SharpLeft),
        new LaneInfo(new LaneWay[] {LaneWay.ReverseLeft, LaneWay.Left}, LaneWay.Left),
        new LaneInfo(new LaneWay[] {LaneWay.ReverseLeft, LaneWay.SlightLeft}, LaneWay.SlightLeft),
    };
    final Bitmap left = render("short_reverse_left", leftLanes);
    Assert.assertEquals(INACTIVE, left.getPixel(64 + 19, 50));
    Assert.assertEquals(Color.BLACK, left.getPixel(64 + 28, 8));

    final Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    final Bitmap carImage = Graphics.drawableToBitmap(new LanesDrawable(context, leftLanes));
    save(context, "lane_glyph_short_reverse_car_image.png", carImage);

    final Bitmap right =
        render("short_reverse_right",
               new LaneInfo[] {
                   new LaneInfo(new LaneWay[] {LaneWay.SlightRight, LaneWay.ReverseRight}, LaneWay.SlightRight),
                   new LaneInfo(new LaneWay[] {LaneWay.Right, LaneWay.ReverseRight}, LaneWay.Right),
                   new LaneInfo(new LaneWay[] {LaneWay.SharpRight, LaneWay.ReverseRight}, LaneWay.SharpRight),
               });
    Assert.assertEquals(INACTIVE, right.getPixel(64 + 45, 50));
    Assert.assertEquals(Color.BLACK, right.getPixel(64 + 36, 8));
    final Bitmap leftActive =
        render("short_reverse_left_active",
               new LaneInfo[] {
                   new LaneInfo(new LaneWay[] {LaneWay.ReverseLeft, LaneWay.SharpLeft}, LaneWay.ReverseLeft),
                   new LaneInfo(new LaneWay[] {LaneWay.ReverseLeft, LaneWay.Left}, LaneWay.ReverseLeft),
                   new LaneInfo(new LaneWay[] {LaneWay.ReverseLeft, LaneWay.SlightLeft}, LaneWay.ReverseLeft),
               });
    Assert.assertEquals(ACTIVE, leftActive.getPixel(64 + 19, 50));

    final Bitmap rightActive =
        render("short_reverse_right_active",
               new LaneInfo[] {
                   new LaneInfo(new LaneWay[] {LaneWay.SlightRight, LaneWay.ReverseRight}, LaneWay.ReverseRight),
                   new LaneInfo(new LaneWay[] {LaneWay.Right, LaneWay.ReverseRight}, LaneWay.ReverseRight),
                   new LaneInfo(new LaneWay[] {LaneWay.SharpRight, LaneWay.ReverseRight}, LaneWay.ReverseRight),
               });
    Assert.assertEquals(ACTIVE, rightActive.getPixel(64 + 45, 50));
  }

  @Test
  public void renderUnrestrictedLanes() throws IOException
  {
    // Unrestricted lanes must not invent a through arrow. Routing may still
    // recommend a turn from one of them.
    final Bitmap unrestricted = render("unrestricted", new LaneInfo[] {
                                                           new LaneInfo(new LaneWay[] {LaneWay.None}, LaneWay.None),
                                                           new LaneInfo(new LaneWay[] {LaneWay.None}, LaneWay.Right),
                                                       });
    Assert.assertEquals(Color.BLACK, unrestricted.getPixel(32, 9));
    Assert.assertEquals(INACTIVE, unrestricted.getPixel(32, 54));
    Assert.assertEquals(ACTIVE, unrestricted.getPixel(64 + 40, 12));
  }

  @Test
  public void renderSingleDirections() throws IOException
  {
    render("single_directions", new LaneInfo[] {
                                    new LaneInfo(new LaneWay[] {LaneWay.ReverseLeft}, LaneWay.ReverseLeft),
                                    new LaneInfo(new LaneWay[] {LaneWay.SharpLeft}, LaneWay.SharpLeft),
                                    new LaneInfo(new LaneWay[] {LaneWay.Left}, LaneWay.Left),
                                    new LaneInfo(new LaneWay[] {LaneWay.SlightLeft}, LaneWay.SlightLeft),
                                    new LaneInfo(new LaneWay[] {LaneWay.Through}, LaneWay.Through),
                                    new LaneInfo(new LaneWay[] {LaneWay.SlightRight}, LaneWay.SlightRight),
                                    new LaneInfo(new LaneWay[] {LaneWay.Right}, LaneWay.Right),
                                    new LaneInfo(new LaneWay[] {LaneWay.SharpRight}, LaneWay.SharpRight),
                                    new LaneInfo(new LaneWay[] {LaneWay.ReverseRight}, LaneWay.ReverseRight),
                                });
  }

  private static void renderTheme(String name, LaneInfo[] lanes, int background, int active) throws IOException
  {
    final Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    final Bitmap bitmap = Bitmap.createBitmap(lanes.length * 32, 32, Bitmap.Config.ARGB_8888);
    final Canvas canvas = new Canvas(bitmap);
    canvas.drawColor(background);
    final LanesDrawable drawable = new LanesDrawable(context, lanes, active, Color.argb(51, 0, 0, 0));
    drawable.setBounds(0, 0, bitmap.getWidth(), bitmap.getHeight());
    drawable.draw(canvas);
    save(context, "lane_glyph_" + name + ".png", bitmap);
  }

  private static Bitmap render(String name, LaneInfo[] lanes) throws IOException
  {
    final Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    final LanesDrawable drawable = new LanesDrawable(context, lanes, ACTIVE, INACTIVE);
    final int intrinsicWidth = drawable.getIntrinsicWidth();

    final Bitmap bitmap = Bitmap.createBitmap(lanes.length * 64, 64, Bitmap.Config.ARGB_8888);
    final Canvas canvas = new Canvas(bitmap);
    canvas.drawColor(Color.BLACK);
    drawable.setBounds(0, 0, bitmap.getWidth(), bitmap.getHeight());
    drawable.draw(canvas);
    save(context, "lane_glyph_" + name + ".png", bitmap);

    final Bitmap small = Bitmap.createBitmap(lanes.length * 32, 32, Bitmap.Config.ARGB_8888);
    final Canvas smallCanvas = new Canvas(small);
    smallCanvas.drawColor(Color.BLACK);
    drawable.setBounds(0, 0, small.getWidth(), small.getHeight());
    drawable.draw(smallCanvas);
    Assert.assertEquals(intrinsicWidth, drawable.getIntrinsicWidth());
    save(context, "lane_glyph_" + name + "_small.png", small);

    return bitmap;
  }

  private static void save(Context context, String name, Bitmap bitmap) throws IOException
  {
    final File file = new File(context.getExternalFilesDir(null), name);
    try (FileOutputStream output = new FileOutputStream(file))
    {
      Assert.assertTrue(bitmap.compress(Bitmap.CompressFormat.PNG, 100, output));
    }
  }
}
