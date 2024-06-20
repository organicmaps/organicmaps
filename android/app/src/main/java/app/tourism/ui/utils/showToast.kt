package app.tourism.ui.utils

import android.content.Context
import android.os.Build
import android.text.Html
import android.text.Html.TO_HTML_PARAGRAPH_LINES_CONSECUTIVE
import android.widget.Toast
import androidx.fragment.app.Fragment

fun Fragment.showToast(text: String) {
    getAppToast(requireContext(), text).show()
}

fun Context.showToast(text: String) {
    getAppToast(this, text).show()
}

private fun getAppToast(context: Context, text: String): Toast {
    val htmlText = "<small>$text</small>"

    return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
        Toast.makeText(
            context,
            Html.fromHtml(htmlText, TO_HTML_PARAGRAPH_LINES_CONSECUTIVE),
            Toast.LENGTH_SHORT
        )
    } else {
        Toast.makeText(
            context,
            Html.fromHtml(htmlText),
            Toast.LENGTH_SHORT
        )
    }
}