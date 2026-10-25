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
import app.organicmaps.BuildConfig;
import app.organicmaps.R;
import org.junit.Test;
import org.mockito.MockedStatic;

public class RoutingTimeFormattingTest
{
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
      assertEquals("1\u00A0min", Utils.formatRoutingTime(context, 0, size).toString());
      assertEquals("1\u00A0min", Utils.formatRoutingTime(context, 20, size).toString());
      assertEquals("1\u00A0min", Utils.formatRoutingTime(context, 59, size).toString());
      assertEquals("1\u00A0min", Utils.formatRoutingTime(context, 60, size).toString());
      assertEquals("1\u00A0min", Utils.formatRoutingTime(context, 89, size).toString());
      assertEquals("2\u00A0min", Utils.formatRoutingTime(context, 90, size).toString());
      assertEquals("4\u00A0min", Utils.formatRoutingTime(context, 269, size).toString());
      assertEquals("5\u00A0min", Utils.formatRoutingTime(context, 270, size).toString());
      assertEquals("27\u00A0min", Utils.formatRoutingTime(context, 1649, size).toString());
      assertEquals("28\u00A0min", Utils.formatRoutingTime(context, 1650, size).toString());
      assertEquals("28\u00A0min", Utils.formatRoutingTime(context, 1665, size).toString());
      assertEquals("35\u00A0min", Utils.formatRoutingTime(context, 2085, size).toString());
      assertEquals("59\u00A0min", Utils.formatRoutingTime(context, 3569, size).toString());
      assertEquals("1\u00A0h", Utils.formatRoutingTime(context, 3570, size).toString());
      assertEquals("1\u00A0h", Utils.formatRoutingTime(context, 3600, size).toString());
      assertEquals("1\u00A0h\u00A01\u00A0min", Utils.formatRoutingTime(context, 3630, size).toString());
      assertEquals("24\u00A0h", Utils.formatRoutingTime(context, 86400, size).toString());
      assertEquals("26\u00A0h\u00A015\u00A0min", Utils.formatRoutingTime(context, 94485, size).toString());
      assertEquals("596523\u00A0h\u00A014\u00A0min",
                   Utils.formatRoutingTime(context, Integer.MAX_VALUE, size).toString());
      assertEquals("0\u00A0min", Utils.formatDuration(context, 0, size).toString());
      assertEquals("4\u00A0min", Utils.formatDuration(context, 269, size).toString());
      assertEquals("27\u00A0min", Utils.formatDuration(context, 1665, size).toString());
      assertEquals("1\u00A0h\u00A00\u00A0min", Utils.formatDuration(context, 3600, size).toString());
    }
  }

  @Test
  public void routingFormatterRejectsNegativeEstimates()
  {
    assumeTrue(BuildConfig.DEBUG);
    Context context = mock(Context.class);
    for (int seconds : new int[] {-1, Integer.MIN_VALUE})
      assertThrows(AssertionError.class,
                   () -> Utils.formatRoutingTime(context, seconds, R.dimen.text_size_routing_number));
  }
}
