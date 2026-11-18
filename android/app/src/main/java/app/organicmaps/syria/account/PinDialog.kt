package app.organicmaps.syria.account

import android.app.Activity
import android.os.Build
import android.view.LayoutInflater
import android.view.autofill.AutofillManager
import android.widget.EditText
import android.widget.Toast
import app.organicmaps.R
import app.organicmaps.sdk.util.log.Logger
import app.organicmaps.syria.ApiClient
import app.organicmaps.syria.ApiException
import app.organicmaps.syria.SyriaPrefs
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import java.io.IOException

/**
 * Sets or changes the 6-digit PIN of the signed-in user: offered after a Telegram or SMS sign-in on an
 * account without a PIN (e.g. a forgotten one) and from the settings. On success the phone's password
 * manager is asked to save the phone number + PIN.
 */
object PinDialog {
    private const val TAG = "SyriaPin"
    private const val PIN_LENGTH = 6

    @JvmStatic
    @JvmOverloads
    fun show(activity: Activity, onDone: Runnable? = null) {
        val view = LayoutInflater.from(activity).inflate(R.layout.dialog_syria_pin, null)
        val phone = view.findViewById<EditText>(R.id.pin_dialog_phone)
        val pin = view.findViewById<EditText>(R.id.pin_dialog_pin)
        phone.setText(SyriaPrefs.phone(activity).orEmpty())

        val dialog = MaterialAlertDialogBuilder(activity)
            .setTitle(R.string.syria_pin_set_title)
            .setView(view)
            .setNegativeButton(R.string.syria_pin_skip) { _, _ -> onDone?.run() }
            .setPositiveButton(R.string.syria_pin_save, null)
            .setCancelable(false)
            .show()
        // Kept open on a bad PIN or an error: the click is handled here instead of by the dialog.
        dialog.getButton(android.app.AlertDialog.BUTTON_POSITIVE).setOnClickListener {
            val value = pin.text.toString()
            if (value.length != PIN_LENGTH || !value.all { it.isDigit() }) {
                pin.error = activity.getString(R.string.syria_pin_invalid)
                return@setOnClickListener
            }
            it.isEnabled = false
            Thread {
                val error = try {
                    ApiClient.setPin(activity, value)
                    null
                } catch (e: ApiException) {
                    e.message ?: e.toString()
                } catch (e: IOException) {
                    Logger.w(TAG, "Setting the PIN failed: $e")
                    activity.getString(R.string.syria_error_network)
                }
                activity.runOnUiThread {
                    if (activity.isFinishing) return@runOnUiThread
                    it.isEnabled = true
                    if (error != null) {
                        pin.error = error
                        return@runOnUiThread
                    }
                    commitAutofill(activity)
                    Toast.makeText(activity, R.string.syria_pin_saved, Toast.LENGTH_SHORT).show()
                    dialog.dismiss()
                    onDone?.run()
                }
            }.start()
        }
    }

    /** Ends the autofill session now, so Android / the password manager offers to save the credentials. */
    @JvmStatic
    fun commitAutofill(activity: Activity) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            activity.getSystemService(AutofillManager::class.java)?.commit()
        }
    }
}
