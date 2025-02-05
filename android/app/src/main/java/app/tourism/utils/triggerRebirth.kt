package app.tourism.utils

import android.content.Context
import android.content.Intent

fun triggerRebirth(context: Context, myClass: Class<*>?) {
    val intent = Intent(context, myClass)
    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK)
    context.startActivity(intent)
    Runtime.getRuntime().exit(0)
}