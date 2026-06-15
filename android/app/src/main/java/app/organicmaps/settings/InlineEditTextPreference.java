package app.organicmaps.settings;

import android.content.Context;
import android.text.Editable;
import android.text.InputType;
import android.text.TextWatcher;
import android.util.AttributeSet;
import android.widget.EditText;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;
import app.organicmaps.R;

// A Preference that edits a short text value inline in the list, without the popup dialog that
// EditTextPreference shows. The value is not persisted by the preference itself: the host fragment
// reads getText() and stores it (here: in the C++ core). Suited for a single field like a tile URL.
public class InlineEditTextPreference extends Preference
{
  @NonNull
  private String mText = "";
  @Nullable
  private CharSequence mHint;

  public InlineEditTextPreference(@NonNull Context context, @Nullable AttributeSet attrs)
  {
    super(context, attrs);
    setLayoutResource(R.layout.pref_inline_edit_text);
    // The row itself is not clickable; taps land on the EditText.
    setSelectable(false);
  }

  public void setText(@Nullable String text)
  {
    mText = text == null ? "" : text;
    notifyChanged();
  }

  @NonNull
  public String getText()
  {
    return mText;
  }

  public void setHint(@Nullable CharSequence hint)
  {
    mHint = hint;
  }

  @Override
  public void onBindViewHolder(@NonNull PreferenceViewHolder holder)
  {
    super.onBindViewHolder(holder);

    final EditText editText = (EditText) holder.findViewById(R.id.edit_text);
    if (editText == null)
      return;

    // Rows are recycled: drop the previous watcher before resetting the text so we don't feed our
    // own programmatic update back into mText.
    final Object tag = editText.getTag();
    if (tag instanceof TextWatcher)
      editText.removeTextChangedListener((TextWatcher) tag);

    editText.setHint(mHint);
    editText.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
    editText.setSingleLine(true);
    editText.setEnabled(isEnabled());
    if (!editText.getText().toString().equals(mText))
      editText.setText(mText);

    final TextWatcher watcher = new TextWatcher() {
      @Override
      public void beforeTextChanged(CharSequence s, int start, int count, int after)
      {}

      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count)
      {}

      @Override
      public void afterTextChanged(Editable s)
      {
        mText = s.toString();
      }
    };
    editText.addTextChangedListener(watcher);
    editText.setTag(watcher);
  }
}
