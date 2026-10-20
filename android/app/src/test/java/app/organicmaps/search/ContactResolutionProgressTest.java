package app.organicmaps.search;

import static org.mockito.Mockito.*;

import android.view.View;
import android.widget.TextView;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.util.List;
import org.junit.Test;

public class ContactResolutionProgressTest
{
  @Test
  public void rowProgressTracksSelectionRefreshCancellationAndRecycling() throws Exception
  {
    final SearchAdapter adapter = new SearchAdapter(mock(SearchFragment.class));
    // Local Android stubs do not initialize the Observable superclass used by RecyclerView.
    final var observable = RecyclerView.Adapter.class.getDeclaredField("mObservable");
    observable.setAccessible(true);
    observable.set(adapter, mock(observable.getType()));
    final ContactAddress first = contact("First Person", "123 Main Street");
    final ContactAddress second = contact("Second Person", "456 Main Street");
    adapter.refreshContactData(List.of(first, second));
    final View row = mock(View.class);
    final View icon = mock(View.class);
    final View progress = mock(View.class);
    when(row.findViewById(R.id.title)).thenReturn(mock(TextView.class));
    when(row.findViewById(R.id.address)).thenReturn(mock(TextView.class));
    when(row.findViewById(R.id.contact_icon)).thenReturn(icon);
    when(row.findViewById(R.id.contact_progress)).thenReturn(progress);
    final Class<?> holderType = Class.forName(SearchAdapter.class.getName() + "$ContactViewHolder");
    final Constructor<?> constructor = holderType.getDeclaredConstructor(SearchAdapter.class, View.class);
    constructor.setAccessible(true);
    final RecyclerView.ViewHolder holder = (RecyclerView.ViewHolder) constructor.newInstance(adapter, row);

    adapter.setResolvingContact(first);
    adapter.onBindViewHolder(holder, 0);
    verify(progress).setVisibility(View.VISIBLE);
    verify(icon).setVisibility(View.INVISIBLE);
    verify(row).setEnabled(false);
    clearInvocations(progress, icon, row);

    adapter.refreshContactData(List.of(contact("First Person", "123 Main Street"), second));
    adapter.onBindViewHolder(holder, 0);
    verify(progress).setVisibility(View.VISIBLE);
    clearInvocations(progress, icon, row);

    adapter.setResolvingContact(second);
    adapter.onBindViewHolder(holder, 0);
    verify(progress).setVisibility(View.GONE);
    verify(icon).setVisibility(View.VISIBLE);
    verify(row).setEnabled(true);
    clearInvocations(progress, icon, row);
    adapter.onBindViewHolder(holder, 1);
    verify(progress).setVisibility(View.VISIBLE);
    clearInvocations(progress, icon, row);

    adapter.setResolvingContact(null);
    adapter.onBindViewHolder(holder, 1);
    verify(progress).setVisibility(View.GONE);
    verify(row).setEnabled(true);
    clearInvocations(progress, icon, row);

    adapter.setResolvingContact(first);
    adapter.clear();
    adapter.refreshContactData(List.of(first));
    adapter.onBindViewHolder(holder, 0);
    verify(progress).setVisibility(View.GONE);
  }

  @Test
  public void clearingPendingSelectionClearsRowProgress() throws Exception
  {
    final SearchFragment fragment = mock(SearchFragment.class, CALLS_REAL_METHODS);
    final SearchAdapter adapter = mock(SearchAdapter.class);
    final var field = SearchFragment.class.getDeclaredField("mSearchAdapter");
    field.setAccessible(true);
    field.set(fragment, adapter);
    final Method clear = SearchFragment.class.getDeclaredMethod("clearPendingContactAddress");
    clear.setAccessible(true);
    clear.invoke(fragment);
    verify(adapter).setResolvingContact(null);
  }

  private static ContactAddress contact(String name, String address)
  {
    return new ContactAddress(name, "", address, address, "Springfield");
  }
}
