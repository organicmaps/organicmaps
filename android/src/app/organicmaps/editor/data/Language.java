package app.organicmaps.editor.data;

import androidx.annotation.NonNull;

// Corresponds to StringUtf8Multilang::Lang in core.
public class Language
{
  // StringUtf8Multilang::GetLangByCode(StringUtf8Multilang::kDefaultCode).
  public static final String DEFAULT_LANG_CODE = "default";

  public final String code;
  public final String name;

  public Language(@NonNull String code, @NonNull String name)
  {
    this.code = code;
    this.name = name;
  }

  public boolean isDefaultLang()
  {
    return code.equals(DEFAULT_LANG_CODE);
  }
}
