package app.organicmaps.editor;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.base.BaseMwmRecyclerFragment;
import app.organicmaps.sdk.bookmarks.data.Metadata;
import app.organicmaps.sdk.editor.Editor;

public class SelfServiceFragment extends BaseMwmRecyclerFragment<SelfServiceAdapter>
{
  private String mSelectedString;

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return super.onCreateView(inflater, container, savedInstanceState);
  }

  @NonNull
  public String getSelection()
  {
    return getAdapter().getSelected();
  }

  @CallSuper
  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    mSelectedString = Editor.nativeGetMetadata(Metadata.MetadataType.FMD_SELF_SERVICE.toInt());
    super.onViewCreated(view, savedInstanceState);
  }

  @NonNull
  @Override
  protected SelfServiceAdapter createAdapter()
  {
    return new SelfServiceAdapter(this, mSelectedString);
  }

  protected void saveSelection(String selection)
  {
    if (getParentFragment() instanceof EditorHostFragment)
      ((EditorHostFragment) getParentFragment()).setSelection(Metadata.MetadataType.FMD_SELF_SERVICE, selection);
  }
}
