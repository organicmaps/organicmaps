package app.organicmaps.widget;


import android.content.Context;
import android.content.res.TypedArray;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.AttributeSet;
import android.widget.EditText;

import androidx.annotation.NonNull;
import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

import app.organicmaps.R;

/**
 * An EditTextPreference alternative that always shows the EditText input field rather
 * than tucking it away inside of a dialog. Lacks summary capability.
 */
public class BetterEditTextPreference extends Preference
{

  private EditText editText;
  private String text;

  public BetterEditTextPreference(Context context)
  {
    this(context, null);
  }

  public BetterEditTextPreference(Context context, AttributeSet attrs)
  {
    this(context, attrs, androidx.preference.R.attr.preferenceStyle);
  }

  public BetterEditTextPreference(Context context, AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
    setLayoutResource(R.layout.preference_always_edit_text);
  }

  @Override
  public void onBindViewHolder(@NonNull PreferenceViewHolder holder)
  {
    super.onBindViewHolder(holder);

    editText = (EditText) holder.findViewById(R.id.et_pref_edit_text);

    holder.itemView.setClickable(false);

    if (editText != null && text != null)
      editText.setText(text);

    if (editText != null) {
      editText.addTextChangedListener(new TextWatcher() {
        @Override
        public void beforeTextChanged(CharSequence s, int start, int count, int after) {
          // unused
        }

        @Override
        public void onTextChanged(CharSequence s, int start, int before, int count) {
          if (s.length() == 1)
            editText.setSelection(editText.getText().length());       // a hacky workaround to a pesky bug only for the scope of the proof-of-concept
        }

        @Override
        public void afterTextChanged(Editable s) {
          String newValue = s.toString();
          if (callChangeListener(newValue)) {
            text = newValue;
            persistString(newValue);
            notifyDependencyChange(shouldDisableDependents());
          }
        }
      });
    }
  }

  @Override
  protected Object onGetDefaultValue(TypedArray a, int index)
  {
    return a.getString(index);
  }

  @Override
  protected void onSetInitialValue(Object defaultValue)
  {
    String defaultValueStr = (defaultValue != null) ? defaultValue.toString() : "";
    text = getPersistedString(defaultValueStr);
  }

  public String getText()
  {
    return text;
  }

  public void setText(String text)
  {
    this.text = text;
    persistString(text);
    notifyChanged();
  }
}