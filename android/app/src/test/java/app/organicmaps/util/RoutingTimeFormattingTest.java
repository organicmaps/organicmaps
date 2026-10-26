package app.organicmaps.util;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThrows;
import static org.junit.Assume.assumeTrue;
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
import org.junit.Test;
import org.mockito.MockedStatic;

public class RoutingTimeFormattingTest
{
  @Test
  public void routingMinutesRoundToNearestWithOneMinuteMinimum()
  {
    int[][] cases = {{0, 1},     {20, 1},    {59, 1},    {60, 1},    {89, 1},       {90, 2},
                     {269, 4},   {270, 5},   {1649, 27}, {1650, 28}, {1665, 28},    {2085, 35},
                     {3569, 59}, {3570, 60}, {3600, 60}, {3630, 61}, {86400, 1440}, {94485, 1575}};
    for (int[] test : cases)
      assertEquals("seconds = " + test[0], test[1], Utils.getRoutingMinutes(test[0]));
    assertEquals(35791394, Utils.getRoutingMinutes(Integer.MAX_VALUE));
  }

  @Test
  public void recordedDurationsKeepTruncatedMinutes()
  {
    Context context = mock(Context.class);
    when(context.getString(R.string.minute)).thenReturn("min");
    when(context.getString(R.string.hour)).thenReturn("h");
    try (MockedStatic<Utils> utils = mockStatic(Utils.class, CALLS_REAL_METHODS);
         MockedStatic<TextUtils> text = mockStatic(TextUtils.class))
    {
      // Replace Android span creation while exercising the real duration formatter.
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
      assertEquals("0\u00A0min", Utils.formatDuration(context, 0, size).toString());
      assertEquals("4\u00A0min", Utils.formatDuration(context, 269, size).toString());
      assertEquals("27\u00A0min", Utils.formatDuration(context, 1665, size).toString());
      assertEquals("1\u00A0h\u00A00\u00A0min", Utils.formatDuration(context, 3600, size).toString());
    }
  }

  @Test
  public void routingMinutesRejectNegativeEstimates()
  {
    assumeTrue(Utils.class.desiredAssertionStatus());
    for (int seconds : new int[] {-1, Integer.MIN_VALUE})
      assertThrows(AssertionError.class, () -> Utils.getRoutingMinutes(seconds));
  }
}
