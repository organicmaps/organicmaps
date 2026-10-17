package app.organicmaps.editor;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertSame;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.text.TextUtils;
import android.view.View;
import android.widget.CompoundButton;
import android.widget.TextView;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.sdk.editor.data.LocalizedStreet;
import org.junit.Test;
import org.mockito.MockedStatic;

public class StreetAdapterTest
{
  /**
   * A second tap queued behind notifyDataSetChanged() runs while its row has no adapter position.
   * https://github.com/organicmaps/organicmaps/issues/8738
   */
  @Test
  public void tapOnRowWithoutPositionSelectsItsStreet()
  {
    final LocalizedStreet first = new LocalizedStreet("Amphitheatre Parkway", "");
    final LocalizedStreet second = new LocalizedStreet("Charleston Road", "");
    final StreetFragment fragment = mock(StreetFragment.class);
    final StreetAdapter adapter = spy(new StreetAdapter(fragment, new LocalizedStreet[] {first, second}, first));
    doNothing().when(adapter).notifyDataSetChanged();

    final StreetAdapter.StreetViewHolder holder = adapter.new StreetViewHolder(newStreetView());
    try (MockedStatic<TextUtils> ignored = mockStatic(TextUtils.class))
    {
      holder.bind(1);
    }
    // Not attached to a RecyclerView, so the row has no position, just like after notifyDataSetChanged().
    assertEquals(RecyclerView.NO_POSITION, holder.getBindingAdapterPosition());

    holder.onClick(holder.itemView);

    assertSame(second, adapter.getSelectedStreet());
    verify(fragment).saveStreet(second);
  }

  private static View newStreetView()
  {
    final View view = mock(View.class);
    when(view.findViewById(R.id.street_default)).thenReturn(mock(TextView.class));
    when(view.findViewById(R.id.street_localized)).thenReturn(mock(TextView.class));
    when(view.findViewById(R.id.selected)).thenReturn(mock(CompoundButton.class));
    return view;
  }
}
