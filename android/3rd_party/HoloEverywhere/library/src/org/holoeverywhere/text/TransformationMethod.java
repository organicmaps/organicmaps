
package org.holoeverywhere.text;

import android.graphics.Rect;
import android.view.View;

public interface TransformationMethod extends android.text.method.TransformationMethod {
    @Override
    public CharSequence getTransformation(CharSequence source, View view);

    @Override
    public void onFocusChanged(View view, CharSequence sourceText,
            boolean focused, int direction,
            Rect previouslyFocusedRect);

    public void setLengthChangesAllowed(boolean allowLengthChanges);
}
