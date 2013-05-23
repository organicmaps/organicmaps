
package org.holoeverywhere.app;

import android.os.Bundle;

public class AlertDialogFragment extends DialogFragment {
    public AlertDialogFragment() {
        setDialogType(DialogType.AlertDialog);
    }

    protected void onCreateDialog(AlertDialog.Builder builder) {

    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        AlertDialog.Builder builder = new AlertDialog.Builder(getSupportActivity());
        onCreateDialog(builder);
        return builder.create();
    }
}
