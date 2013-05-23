
package org.holoeverywhere;

import org.holoeverywhere.app.Application;

import android.content.ClipData;
import android.content.Intent;
import android.os.Build.VERSION;
import android.os.Build.VERSION_CODES;

public final class IntentCompat {
    /**
     * Declare chooser activity in manifest:
     *
     * <pre>
     *  &lt;activity android:name="org.holoeverywhere.ChooserActivity"
     *      android:theme="@style/Holo.Theme.Dialog.Alert.Light"
     *      android:finishOnCloseSystemDialogs="true"
     *      android:excludeFromRecents="true"
     *      android:multiprocess="true" /&gt;
     * </pre>
     */
    public static Intent createChooser(Intent target, CharSequence title) {
        if (VERSION.SDK_INT >= VERSION_CODES.JELLY_BEAN) {
            return Intent.createChooser(target, title);
        }
        Intent intent = new Intent();
        intent.setClass(Application.getLastInstance(), ChooserActivity.class);
        intent.putExtra(Intent.EXTRA_INTENT, target);
        if (title != null) {
            intent.putExtra(Intent.EXTRA_TITLE, title);
        }
        int permFlags = target.getFlags()
                & (Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        if (permFlags != 0) {
            ClipData targetClipData = target.getClipData();
            if (targetClipData == null && target.getData() != null) {
                ClipData.Item item = new ClipData.Item(target.getData());
                String[] mimeTypes;
                if (target.getType() != null) {
                    mimeTypes = new String[] {
                            target.getType()
                    };
                } else {
                    mimeTypes = new String[] {};
                }
                targetClipData = new ClipData(null, mimeTypes, item);
            }
            if (targetClipData != null) {
                intent.setClipData(targetClipData);
                intent.addFlags(permFlags);
            }
        }
        return intent;
    }

    private IntentCompat() {
    }
}
