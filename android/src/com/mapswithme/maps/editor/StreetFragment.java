package com.mapswithme.maps.editor;

import android.os.Bundle;
import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.base.BaseMwmRecyclerFragment;
import com.mapswithme.maps.dialog.EditTextDialogFragment;
import com.mapswithme.maps.editor.data.LocalizedStreet;

public class StreetFragment extends BaseMwmRecyclerFragment<StreetAdapter>
    implements EditTextDialogFragment.EditTextDialogInterface
{
  private LocalizedStreet mSelectedString;

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return super.onCreateView(inflater, container, savedInstanceState);
  }

  @CallSuper
  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    mSelectedString = Editor.nativeGetStreet();
    super.onViewCreated(view, savedInstanceState);
  }

  @Override
  public void onSaveInstanceState(Bundle outState)
  {
    super.onSaveInstanceState(outState);
    Editor.nativeSetStreet(getStreet());
  }

  @NonNull
  @Override
  protected StreetAdapter createAdapter()
  {
    return new StreetAdapter(this, Editor.nativeGetNearbyStreets(), mSelectedString);
  }

  @NonNull
  public LocalizedStreet getStreet()
  {
    return ((StreetAdapter) getAdapter()).getSelectedStreet();
  }

  protected void saveStreet(LocalizedStreet street)
  {
    if (getParentFragment() instanceof EditorHostFragment)
      ((EditorHostFragment) getParentFragment()).setStreet(street);
  }

  @NonNull
  @Override
  public EditTextDialogFragment.OnTextSaveListener getSaveTextListener()
  {
    return text -> saveStreet(new LocalizedStreet(text, ""));
  }

  @NonNull
  @Override
  public EditTextDialogFragment.Validator getValidator()
  {
    return (activity, text) -> !TextUtils.isEmpty(text);
  }
}
