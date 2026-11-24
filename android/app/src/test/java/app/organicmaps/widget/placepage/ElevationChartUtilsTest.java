package app.organicmaps.widget.placepage;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import app.organicmaps.widget.placepage.ElevationChartUtils.YAxisBounds;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.highlight.Highlight;
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

  @Test
  public void highlightIsClampedToTheProfileEnds()
  {
    List<Entry> entries = List.of(new Entry(0f, 100f), new Entry(400f, 300f), new Entry(1000f, 0f));

    assertHighlight(ElevationChartUtils.highlightAt(entries, -5f), 0f, 100f);
    assertHighlight(ElevationChartUtils.highlightAt(entries, 1200f), 1000f, 0f);
  }

  @Test
  public void highlightInsideTheProfileIsInterpolated()
  {
    List<Entry> entries = List.of(new Entry(0f, 100f), new Entry(400f, 300f), new Entry(1000f, 0f));

    assertHighlight(ElevationChartUtils.highlightAt(entries, 200f), 200f, 200f);
    assertHighlight(ElevationChartUtils.highlightAt(entries, 400f), 400f, 300f);
    assertHighlight(ElevationChartUtils.highlightAt(entries, 700f), 700f, 150f);
  }

  @Test
  public void chartXOfTheSelectedPointKeepsItsExactDistance()
  {
    // A gap entry, and one whose float would pass the finish of the 999999.995 m track.
    final double gap = 100.16474326527293;
    final double[] data = profile(0.0, gap, gap, 999999.995);
    for (double selected : new double[] {gap, 999999.975})
    {
      final float x = (float) selected;
      assertEquals(selected, ElevationChartUtils.toDistance(x, selected, data), 0.0);
      final float previous = Math.nextDown(x);
      assertEquals(previous, ElevationChartUtils.toDistance(previous, selected, data), 0.0);
    }
    assertTrue(Double.isNaN(ElevationChartUtils.toDistance(Float.NaN, 5.0, profile(0.0, 5.0))));
    assertTrue(Double.isNaN(ElevationChartUtils.toDistance(Float.NaN, Double.NaN, profile())));
  }

  @Test
  public void chartXOfAProfilePointKeepsItsExactDistance()
  {
    // The float of this gap is 2.25 cm into the second section, while the core picks the first one by the exact
    // distance. The next point shares that float too.
    final double gap = 1000000.04;
    final double[] data = profile(0.0, 400.0, gap, gap, 1000000.05, 1000100.0);
    assertEquals(1000000.0625f, (float) gap, 0f);
    assertEquals(gap, ElevationChartUtils.toDistance((float) gap, 400.0, data), 0.0);
    assertEquals(gap, ElevationChartUtils.toDistance((float) gap, Double.NaN, data), 0.0);
    assertEquals(0.0, ElevationChartUtils.toDistance(0f, 400.0, data), 0.0);
    assertEquals(1000100.0, ElevationChartUtils.toDistance(1000100f, 400.0, data), 0.0);
    assertEquals(999999.995, ElevationChartUtils.toDistance(1000000f, Double.NaN, profile(0.0, 999999.995)), 0.0);
    // The finish of a place-page chart without a selection to prefer.
    final double finish = 1001853.8972297852;
    assertEquals(1001853.875f, (float) finish, 0f);
    assertEquals(finish, ElevationChartUtils.toDistance((float) finish, Double.NaN, profile(0.0, finish)), 0.0);
    // The right edge is the finish, not the earlier point with the same float.
    final double[] edge = profile(0.0, 2115024.894151767, 2115025.1067818934);
    assertEquals(2115025.0f, (float) edge[1], 0f);
    assertEquals(2115025.0f, (float) edge[2], 0f);
    assertEquals(edge[2], ElevationChartUtils.toDistance(2115025.0f, Double.NaN, edge), 0.0);
    // Points the chart did not select itself stay as they are.
    assertEquals(200.0, ElevationChartUtils.toDistance(200f, 400.0, data), 0.0);
  }

  private static double[] profile(double... distances)
  {
    return distances;
  }

  private static void assertHighlight(Highlight highlight, float x, float y)
  {
    assertEquals(x, highlight.getX(), EPS);
    assertEquals(y, highlight.getY(), EPS);
    assertEquals(0, highlight.getDataSetIndex());
  }

  private static void assertBounds(YAxisBounds bounds, float lower, float upper)
  {
    assertEquals(lower, bounds.lower(), EPS);
    assertEquals(upper, bounds.upper(), EPS);
  }
}
