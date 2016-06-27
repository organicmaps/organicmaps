package com.mapswithme.maps.editor;

import android.os.Bundle;
import android.support.v7.widget.RecyclerView;

import com.mapswithme.maps.base.BaseMwmRecyclerFragment;
import com.mapswithme.maps.editor.data.Language;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.HashSet;

import static java.util.Collections.sort;

public class LanguagesFragment extends BaseMwmRecyclerFragment
{
  public interface Listener
  {
    void onLanguageSelected(Language language);
  }

  @Override
  protected RecyclerView.Adapter createAdapter()
  {
    Bundle args = getArguments();
    HashSet<String> existingLanguages = new HashSet<>(Arrays.asList(args.getStringArray(EditorHostFragment.kExistingLocalizedNames)));

    ArrayList<Language> languages = new ArrayList<>();
    for (Language lang : Editor.nativeGetSupportedLanguages())
    {
      if (existingLanguages.contains(lang.code))
        continue;
      languages.add(lang);
    }

    sort(languages, new Comparator<Language>() {
      @Override
      public int compare(Language lhs, Language rhs) {
        return lhs.name.compareTo(rhs.name);
      }
    });

    return new LanguagesAdapter(this, languages.toArray(new Language[languages.size()]));
  }

  protected void onLanguageSelected(Language language)
  {
    if (getParentFragment() instanceof Listener)
      ((Listener) getParentFragment()).onLanguageSelected(language);
  }
}
