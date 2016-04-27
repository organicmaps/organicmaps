package com.mapswithme.maps.editor;

import android.support.v7.widget.RecyclerView;

import com.mapswithme.maps.base.BaseMwmRecyclerFragment;
import com.mapswithme.maps.editor.data.Language;

public class LanguagesFragment extends BaseMwmRecyclerFragment
{
  public interface Listener
  {
    void onLanguageSelected(Language language);
  }

  @Override
  protected RecyclerView.Adapter createAdapter()
  {
    return new LanguagesAdapter(this, Editor.nativeGetSupportedLanguages());
  }

  protected void onLanguageSelected(Language language)
  {
    if (getActivity() instanceof Listener)
      ((Listener) getActivity()).onLanguageSelected(language);
    else if (getParentFragment() instanceof Listener)
      ((Listener) getParentFragment()).onLanguageSelected(language);
  }
}
