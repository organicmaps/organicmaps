package app.organicmaps.editor;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.view.ViewTreeObserver;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.sdk.editor.data.LocalizedName;
import app.organicmaps.util.InputUtils;
import app.organicmaps.util.UiUtils;
import com.google.android.material.textfield.TextInputEditText;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.MockedStatic;

public class EditorNamesValidationTest
{
  @Test
  public void allValidNamesPassValidation()
  {
    final MultilanguageAdapter adapter = mock(MultilanguageAdapter.class);
    when(adapter.getItemCount()).thenReturn(3);
    when(adapter.getNameAtPos(0)).thenReturn(new LocalizedName(0, "Name 1", "en", "English"));
    when(adapter.getNameAtPos(1)).thenReturn(new LocalizedName(0, "Name 2", "de", "German"));
    when(adapter.getNameAtPos(2)).thenReturn(new LocalizedName(0, "Name 3", "fr", "French"));

    final RecyclerView namesView = mock(RecyclerView.class);

    assertTrue(EditorFragment.validateNames(adapter, namesView, name -> true));
    verify(namesView, never()).scrollToPosition(anyInt());
  }

  @Test
  public void invalidNameWithAttachedRowFocusesInputField()
  {
    final MultilanguageAdapter adapter = mock(MultilanguageAdapter.class);
    when(adapter.getItemCount()).thenReturn(2);
    when(adapter.getNameAtPos(0)).thenReturn(new LocalizedName(0, "Valid 0", "en", "English"));
    when(adapter.getNameAtPos(1)).thenReturn(new LocalizedName(0, "", "en", "English"));

    final RecyclerView namesView = mock(RecyclerView.class);
    final MultilanguageAdapter.Holder holder = mockHolder();
    when(namesView.findViewHolderForAdapterPosition(1)).thenReturn(holder);

    try (MockedStatic<InputUtils> keyboard = mockStatic(InputUtils.class);
         MockedStatic<UiUtils> uiUtils = mockStatic(UiUtils.class))
    {
      assertFalse(EditorFragment.validateNames(adapter, namesView, name -> !name.isEmpty()));
      verify(namesView).scrollToPosition(1);
      verify(holder.input).requestFocus();
      keyboard.verify(() -> InputUtils.showKeyboard(holder.input));
      uiUtils.verify(() -> UiUtils.waitLayout(any(), any()), never());
    }
  }

  @Test
  public void invalidNameWithDetachedRowFocusesInputFieldAfterLayout()
  {
    final MultilanguageAdapter adapter = mock(MultilanguageAdapter.class);
    when(adapter.getItemCount()).thenReturn(5);
    when(adapter.getNameAtPos(0)).thenReturn(new LocalizedName(0, "Valid 0", "en", "English"));
    when(adapter.getNameAtPos(1)).thenReturn(new LocalizedName(0, "Valid 1", "en", "English"));
    when(adapter.getNameAtPos(2)).thenReturn(new LocalizedName(0, "Valid 2", "en", "English"));
    when(adapter.getNameAtPos(3)).thenReturn(new LocalizedName(0, "Valid 3", "en", "English"));
    when(adapter.getNameAtPos(4)).thenReturn(new LocalizedName(0, "", "en", "English"));

    final RecyclerView namesView = mock(RecyclerView.class);
    when(namesView.findViewHolderForAdapterPosition(4)).thenReturn(null);

    try (MockedStatic<InputUtils> keyboard = mockStatic(InputUtils.class);
         MockedStatic<UiUtils> uiUtils = mockStatic(UiUtils.class))
    {
      assertFalse(EditorFragment.validateNames(adapter, namesView, name -> !name.isEmpty()));
      verify(namesView).scrollToPosition(4);
      keyboard.verify(() -> InputUtils.showKeyboard(any()), never());

      final ArgumentCaptor<ViewTreeObserver.OnGlobalLayoutListener> onLayout =
          ArgumentCaptor.forClass(ViewTreeObserver.OnGlobalLayoutListener.class);
      uiUtils.verify(() -> UiUtils.waitLayout(eq(namesView), onLayout.capture()));

      final MultilanguageAdapter.Holder holder = mockHolder();
      when(namesView.findViewHolderForAdapterPosition(4)).thenReturn(holder);
      onLayout.getValue().onGlobalLayout();

      verify(holder.input).requestFocus();
      keyboard.verify(() -> InputUtils.showKeyboard(holder.input));
    }
  }

  private static MultilanguageAdapter.Holder mockHolder()
  {
    final MultilanguageAdapter.Holder holder = mock(MultilanguageAdapter.Holder.class);
    holder.input = mock(TextInputEditText.class);
    return holder;
  }
}
