package app.organicmaps.widget.placepage;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import app.organicmaps.widget.placepage.ElevationChartUtils.YAxisBounds;
import com.github.mikephil.charting.data.Entry;
import java.util.List;
import org.junit.Test;

public class ElevationChartUtilsTest
{
  private static final float METRIC_STEP = 50f;
  private static final float IMPERIAL_STEP = 30.48f;
  private static final float EPS = 0.01f;

  private static YAxisBounds metric(float min, float max)
  {
    return ElevationChartUtils.computeYAxisBounds(min, max, METRIC_STEP);
  }

  /** Share of the axis height the highest point sits at, 1.0 meaning the very top border. */
  private static float peakPosition(YAxisBounds bounds, float max)
  {
    return (max - bounds.lower()) / (bounds.upper() - bounds.lower());
  }

  @Test
  public void nearlyFlatTrackIsNotGluedToTheTop()
  {
    // 10% of a 1m range rounds down to zero padding, so rounding to the 50m grid used to produce
    // 50..100 and draw the profile at 98% of the chart height, leaving no room for the markers.
    YAxisBounds bounds = metric(98f, 99f);

    assertEquals(50f, bounds.lower(), EPS);
    assertEquals(150f, bounds.upper(), EPS);
    assertEquals(0.49f, peakPosition(bounds, 99f), EPS);
  }

  @Test
  public void subMetreRangeGetsTightBounds()
  {
    // A sub-metre range used to take a dedicated branch padding by a whole step on each side, which
    // spread a half-metre track over a 150m axis.
    YAxisBounds bounds = metric(98.5f, 99f);

    assertEquals(50f, bounds.lower(), EPS);
    assertEquals(150f, bounds.upper(), EPS);
  }

  @Test
  public void completelyFlatTrackIsCentered()
  {
    YAxisBounds bounds = metric(100f, 100f);

    assertEquals(50f, bounds.lower(), EPS);
    assertEquals(150f, bounds.upper(), EPS);
  }

  @Test
  public void rangeWiderThanAStepIsLeftAlone()
  {
    // The minimum padding is negative once the range exceeds a step, so these keep the plain 10%.
    assertBounds(metric(120f, 180f), 100f, 200f);
    assertBounds(metric(100f, 800f), 0f, 1000f);
  }

  @Test
  public void wideRangeDoublesTheStepToCapLabelCount()
  {
    YAxisBounds bounds = metric(1200f, 2400f);

    assertBounds(bounds, 1050f, 2650f);
    assertEquals(4, bounds.stepCount());
  }

  @Test
  public void altitudesBelowSeaLevelAreSupported()
  {
    assertBounds(metric(-5f, 3f), -50f, 50f);
  }

  @Test
  public void imperialStepIsHonoured()
  {
    // The same 1m range used to give 91.44..152.4 in feet, drawing the profile at 12% of the chart
    // height, this time glued to the bottom.
    YAxisBounds bounds = ElevationChartUtils.computeYAxisBounds(98f, 99f, IMPERIAL_STEP);

    assertEquals(2 * IMPERIAL_STEP, bounds.lower(), EPS);
    assertEquals(4 * IMPERIAL_STEP, bounds.upper(), EPS);
    assertEquals(0.62f, peakPosition(bounds, 99f), EPS);
  }

  @Test
  public void dataAlwaysFitsInsideTheAxis()
  {
    for (float step : new float[] {METRIC_STEP, IMPERIAL_STEP})
      for (float min = -300f; min <= 3000f; min += 37f)
        for (float range : new float[] {0f, 0.5f, 1f, 5f, 42f, 60f, 700f, 2000f})
        {
          float max = min + range;
          YAxisBounds bounds = ElevationChartUtils.computeYAxisBounds(min, max, step);
          String where = "min=" + min + " max=" + max + " step=" + step;

          assertTrue(where, bounds.lower() <= min);
          assertTrue(where, bounds.upper() >= max);
          assertTrue(where, bounds.upper() > bounds.lower());
          assertTrue(where, bounds.stepCount() >= 1);
          assertTrue(where, bounds.stepCount() <= 6);
        }
  }

  @Test
  public void interpolationReturnsZeroForEmptyTrack()
  {
    assertEquals(0f, ElevationChartUtils.interpolateY(List.of(), 10f), EPS);
  }

  @Test
  public void interpolationReturnsOnlyAltitudeForSinglePoint()
  {
    List<Entry> entries = List.of(new Entry(10f, 120f));

    assertEquals(120f, ElevationChartUtils.interpolateY(entries, 0f), EPS);
    assertEquals(120f, ElevationChartUtils.interpolateY(entries, 10f), EPS);
    assertEquals(120f, ElevationChartUtils.interpolateY(entries, 20f), EPS);
  }

  @Test
  public void interpolationClampsBeforeFirstPoint()
  {
    List<Entry> entries = List.of(new Entry(10f, 100f), new Entry(30f, 200f));

    assertEquals(100f, ElevationChartUtils.interpolateY(entries, 0f), EPS);
  }

  @Test
  public void interpolationClampsAfterLastPoint()
  {
    List<Entry> entries = List.of(new Entry(10f, 100f), new Entry(30f, 200f));

    assertEquals(200f, ElevationChartUtils.interpolateY(entries, 40f), EPS);
  }

  @Test
  public void interpolationHandlesAscendingDescendingAndFlatSegments()
  {
    List<Entry> entries = List.of(new Entry(10f, 100f), new Entry(30f, 200f), new Entry(70f, 0f), new Entry(90f, 0f));

    assertEquals(125f, ElevationChartUtils.interpolateY(entries, 15f), EPS);
    assertEquals(150f, ElevationChartUtils.interpolateY(entries, 40f), EPS);
    assertEquals(0f, ElevationChartUtils.interpolateY(entries, 80f), EPS);
  }

  @Test
  public void interpolationPreservesAltitudesAtExactPoints()
  {
    List<Entry> entries = List.of(new Entry(10f, 100f), new Entry(30f, 200f), new Entry(70f, -20f));

    assertEquals(100f, ElevationChartUtils.interpolateY(entries, 10f), EPS);
    assertEquals(200f, ElevationChartUtils.interpolateY(entries, 30f), EPS);
    assertEquals(-20f, ElevationChartUtils.interpolateY(entries, 70f), EPS);
  }

  @Test
  public void interpolationHandlesRepeatedDistances()
  {
    List<Entry> entries = List.of(new Entry(0f, 0f), new Entry(10f, 100f), new Entry(10f, 200f), new Entry(20f, 300f));

    assertEquals(50f, ElevationChartUtils.interpolateY(entries, 5f), EPS);
    // At the shared distance, keep the first altitude; beyond it, use the outgoing segment.
    assertEquals(100f, ElevationChartUtils.interpolateY(entries, 10f), EPS);
    assertEquals(250f, ElevationChartUtils.interpolateY(entries, 15f), EPS);
  }

  @Test
  public void interpolationUsesPreviousAltitudeForTinySegment()
  {
    List<Entry> entries = List.of(new Entry(0f, 100f), new Entry(5e-10f, 200f));

    assertEquals(100f, ElevationChartUtils.interpolateY(entries, 2.5e-10f), EPS);
  }

  private static void assertBounds(YAxisBounds bounds, float lower, float upper)
  {
    assertEquals(lower, bounds.lower(), EPS);
    assertEquals(upper, bounds.upper(), EPS);
  }
}
