
package org.holoeverywhere.internal;

import org.holoeverywhere.widget.EditText;

import android.content.Context;
import android.util.AttributeSet;
import android.view.inputmethod.EditorInfo;

public class NumberPickerEditText extends EditText {
    public NumberPickerEditText(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public void onEditorAction(int actionCode) {
        super.onEditorAction(actionCode);
        if (actionCode == EditorInfo.IME_ACTION_DONE) {
            clearFocus();
        }
    }
}
