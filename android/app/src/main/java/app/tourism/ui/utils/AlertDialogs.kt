package app.tourism.ui.utils

import android.content.Context
import app.organicmaps.R

fun showYesNoAlertDialog(context: Context, title: String, onPositiveButtonClick: () -> Unit) {
    android.app.AlertDialog.Builder(context)
        .setMessage(title)
        .setNegativeButton(context.getString(R.string.no)) { _, _ -> }
        .setPositiveButton(context.getString(R.string.yes)) { _, _ ->
            onPositiveButtonClick()
        }
        .create().show()
}

fun showMessageInAlertDialog(context: Context, title: String, message: String) {
    android.app.AlertDialog.Builder(context)
        .setTitle(title)
        .setMessage(message)
        .setPositiveButton(context.getString(R.string.ok)) { _, _ -> }
        .create().show()
}
