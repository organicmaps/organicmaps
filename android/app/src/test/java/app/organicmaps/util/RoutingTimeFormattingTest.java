package app.organicmaps.util;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.text.SpannableStringBuilder;
import android.text.TextUtils;
import app.organicmaps.R;
import java.text.Bidi;
import org.junit.Test;
import org.mockito.MockedStatic;

public class RoutingTimeFormattingTest
{
  @Test
  public void roundsAtHalfMinuteBoundaries()
  {
    int[][] cases = {{60, 1},    {269, 4},   {270, 5},   {1649, 27}, {1650, 28},    {1665, 28},   {2085, 35},
                     {3569, 59}, {3570, 60}, {3600, 60}, {3630, 61}, {86400, 1440}, {94485, 1575}};
    for (int[] test : cases)
      assertEquals(test[0] + " seconds", test[1], Utils.getRoundedRoutingTimeMinutes(test[0]));
  }

  @Test
  public void displaysSubMinuteDurationsWithoutZeroMinutes()
  {
    for (int seconds = 0; seconds < 60; ++seconds)
      assertEquals("\u2066<1\u2069", Utils.getRoutingMinuteValue(seconds));
    assertEquals("1", Utils.getRoutingMinuteValue(60));
    assertEquals("0", Utils.getRoutingMinuteValue(3570));
    assertEquals("15", Utils.getRoutingMinuteValue(94485));
  }

  @Test
  public void roundingDoesNotOverflowAtLargestIntegerDuration()
  {
    assertEquals(35791394L, Utils.getRoundedRoutingTimeMinutes(Integer.MAX_VALUE));
  }

  @Test
  public void lessThanComparisonRemainsLeftToRightWithRtlUnits()
  {
    for (String unit : new String[] {"min", "د", "ד׳", "دقیقه"})
    {
      Bidi bidi = new Bidi(Utils.getRoutingMinuteValue(20) + "\u00A0" + unit, Bidi.DIRECTION_DEFAULT_LEFT_TO_RIGHT);
      assertEquals(unit, 0, bidi.getLevelAt(1) % 2);
    }
  }

  @Test
  public void routeAndRecordedDurationsUseTheirOwnPolicies()
  {
    Context context = mock(Context.class);
    when(context.getString(R.string.minute)).thenReturn("min");
    when(context.getString(R.string.hour)).thenReturn("h");
    try (MockedStatic<Utils> utils = mockStatic(Utils.class, CALLS_REAL_METHODS);
         MockedStatic<TextUtils> text = mockStatic(TextUtils.class))
    {
      // Replace Android span creation while exercising the real duration formatters.
      utils.when(() -> Utils.formatTime(any(Context.class), anyInt(), anyInt(), anyString(), anyString()))
          .thenAnswer(invocation -> {
            SpannableStringBuilder result = mock(SpannableStringBuilder.class);
            String value = invocation.getArgument(3) + "\u00A0" + invocation.getArgument(4);
            when(result.toString()).thenReturn(value);
            return result;
          });
      text.when(() -> TextUtils.concat(any(CharSequence[].class))).thenAnswer(invocation -> {
        StringBuilder result = new StringBuilder();
        for (CharSequence part : (CharSequence[]) invocation.getRawArguments()[0])
          result.append(part.toString());
        return result.toString();
      });

      int size = R.dimen.text_size_routing_number;
      assertEquals("\u2066<1\u2069\u00A0min", Utils.formatRoutingTime(context, 45, size).toString());
      assertEquals("4\u00A0min", Utils.formatRoutingTime(context, 269, size).toString());
      assertEquals("5\u00A0min", Utils.formatRoutingTime(context, 270, size).toString());
      assertEquals("28\u00A0min", Utils.formatRoutingTime(context, 1665, size).toString());
      assertEquals("35\u00A0min", Utils.formatRoutingTime(context, 2085, size).toString());
      assertEquals("1\u00A0h", Utils.formatRoutingTime(context, 3570, size).toString());
      assertEquals("1\u00A0h\u00A01\u00A0min", Utils.formatRoutingTime(context, 3630, size).toString());
      assertEquals("26\u00A0h\u00A015\u00A0min", Utils.formatRoutingTime(context, 94485, size).toString());
      assertEquals("0\u00A0min", Utils.formatDuration(context, 0, size).toString());
      assertEquals("4\u00A0min", Utils.formatDuration(context, 269, size).toString());
      assertEquals("27\u00A0min", Utils.formatDuration(context, 1665, size).toString());
      assertEquals("1\u00A0h\u00A00\u00A0min", Utils.formatDuration(context, 3600, size).toString());
    }
  }
}
