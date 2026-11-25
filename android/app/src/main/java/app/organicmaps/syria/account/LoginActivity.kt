package app.organicmaps.syria.account

import android.content.ActivityNotFoundException
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import android.view.View
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.widget.Toolbar
import androidx.core.view.ViewCompat
import androidx.lifecycle.Lifecycle
import app.organicmaps.BuildConfig
import app.organicmaps.R
import app.organicmaps.base.BaseMwmFragmentActivity
import app.organicmaps.sdk.util.log.Logger
import app.organicmaps.syria.ApiClient
import app.organicmaps.syria.ApiException
import app.organicmaps.syria.NarrationDialog
import app.organicmaps.syria.SyriaFormat
import app.organicmaps.syria.tracking.SyncWorker
import app.organicmaps.syria.tracking.TrackingActivity
import app.organicmaps.util.UiUtils
import app.organicmaps.util.Utils
import app.organicmaps.util.WindowInsetUtils
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import java.io.IOException
import java.util.concurrent.Executors
import org.json.JSONObject

/**
 * Sign-in: phone number + 6-digit PIN (a new number registers right away), Telegram (also the way back
 * after a forgotten PIN) or anonymously (per device, no phone number). Opens the tracking screen afterwards
 * unless told otherwise.
 */
class LoginActivity : BaseMwmFragmentActivity() {
    private val executor = Executors.newSingleThreadExecutor()

    private lateinit var countryInput: EditText
    private lateinit var phoneInput: EditText
    private lateinit var pinInput: EditText
    private lateinit var signInButton: Button
    private lateinit var progress: View
    private lateinit var error: TextView
    private lateinit var info: TextView

    private lateinit var anonymousButton: Button

    // Telegram sign-in in progress: collected by polling while this screen is in front.
    private lateinit var telegramButton: Button
    private val handler = Handler(Looper.getMainLooper())
    private var telegramLoginId: String? = null
    private var telegramSecret: String? = null
    private var telegramDeadline = 0L
    private val pollRunnable = Runnable { pollTelegram() }

    override fun getContentLayoutResId(): Int = R.layout.activity_syria_login

    override fun onSafeCreate(savedInstanceState: Bundle?) {
        super.onSafeCreate(savedInstanceState)
        val toolbar = findViewById<Toolbar>(R.id.toolbar)
        toolbar.setTitle(R.string.syria_login_title)
        UiUtils.showHomeUpButton(toolbar)
        toolbar.setNavigationOnClickListener { finish() }
        ViewCompat.setOnApplyWindowInsetsListener(toolbar, WindowInsetUtils.PaddingInsetsListener.excludeBottom())
        ViewCompat.setOnApplyWindowInsetsListener(
            findViewById(R.id.login_scroll),
            WindowInsetUtils.PaddingInsetsListener.excludeTop(),
        )

        countryInput = findViewById(R.id.login_country)
        phoneInput = findViewById(R.id.login_phone)
        pinInput = findViewById(R.id.login_pin)
        signInButton = findViewById(R.id.login_sign_in)
        progress = findViewById(R.id.login_progress)
        error = findViewById(R.id.login_error)
        info = findViewById(R.id.login_info)
        telegramButton = findViewById(R.id.login_telegram)
        anonymousButton = findViewById(R.id.login_anonymous)

        signInButton.setOnClickListener { signInWithPin() }
        telegramButton.setOnClickListener { startTelegram() }
        anonymousButton.setOnClickListener { signInAnonymously() }
        findViewById<View>(R.id.login_privacy).setOnClickListener { Utils.openUrl(this, BuildConfig.SYRIA_PRIVACY_URL) }
        // Instructions: by themselves on the first visit, then with the "?" icon.
        NarrationDialog.attach(this, toolbar, NarrationDialog.LOGIN, savedInstanceState == null)
    }

    override fun onResume() {
        super.onResume()
        // Back from Telegram: check right away whether the number was shared.
        if (telegramLoginId != null) schedulePoll(0)
    }

    override fun onPause() {
        handler.removeCallbacks(pollRunnable)
        super.onPause()
    }

    override fun onSafeDestroy() {
        handler.removeCallbacks(pollRunnable)
        executor.shutdownNow()
        super.onSafeDestroy()
    }

    /** The number from the country code and number fields, or null (with the error shown) if too short. */
    private fun typedPhone(): String? {
        val typed = phoneInput.text.toString()
        if (typed.count { it.isDigit() } < MIN_PHONE_LENGTH) {
            error.setText(R.string.syria_login_invalid_phone)
            return null
        }
        return SyriaFormat.normalizePhoneInput(typed, countryInput.text.toString())
    }

    // ---- phone + PIN

    private fun signInWithPin() {
        val phone = typedPhone() ?: return
        val pin = pinInput.text.toString()
        if (pin.length != PIN_LENGTH || !pin.all { it.isDigit() }) {
            error.setText(R.string.syria_pin_invalid)
            return
        }
        setBusy(true)
        executor.execute {
            try {
                val res = ApiClient.pinLogin(this, phone, pin)
                SyncWorker.enqueueNow(this)
                runOnUiThread {
                    if (isFinishing) return@runOnUiThread
                    // The fields hold the number and PIN: Android / the password manager offers to save them.
                    PinDialog.commitAutofill(this)
                    if (res.optBoolean("created")) {
                        Toast.makeText(this, R.string.syria_pin_account_created, Toast.LENGTH_LONG).show()
                    }
                    finishSignedIn()
                }
            } catch (e: ApiException) {
                runOnUiThread { onPinRefused(e) }
            } catch (e: IOException) {
                Logger.w(TAG, "PIN sign-in failed: $e")
                showError(getString(R.string.syria_error_network))
            }
        }
    }

    private fun onPinRefused(e: ApiException) {
        if (isFinishing) return
        setBusy(false)
        when (e.status) {
            HTTP_UNAUTHORIZED -> error.setText(R.string.syria_pin_wrong)
            HTTP_CONFLICT -> {
                error.text = ""
                MaterialAlertDialogBuilder(this)
                    .setMessage(R.string.syria_pin_not_set)
                    .setNegativeButton(R.string.syria_cancel, null)
                    .setPositiveButton(R.string.syria_login_telegram) { _, _ -> startTelegram() }
                    .show()
            }
            HTTP_TOO_MANY -> {
                val minutes = Regex("(\\d+) min").find(e.message.orEmpty())?.groupValues?.get(1)?.toIntOrNull()
                error.text = getString(R.string.syria_pin_locked, minutes ?: DEFAULT_LOCK_MINUTES)
            }
            HTTP_BAD_REQUEST -> error.setText(R.string.syria_login_invalid_phone)
            else -> error.text = e.message ?: e.toString()
        }
    }

    // ---- anonymous

    private fun signInAnonymously() {
        setBusy(true)
        executor.execute {
            try {
                ApiClient.deviceLogin(this)
                // Uploads what was queued before (searches, map updates) under this device.
                SyncWorker.enqueueNow(this)
                runOnUiThread { finishSignedIn() }
            } catch (e: ApiException) {
                showError(e.message ?: e.toString())
            } catch (e: IOException) {
                Logger.w(TAG, "Anonymous sign-in failed: $e")
                showError(getString(R.string.syria_error_network))
            }
        }
    }

    // ---- Telegram

    private fun startTelegram() {
        setBusy(true)
        executor.execute {
            try {
                val res = ApiClient.startTelegram()
                runOnUiThread {
                    if (isFinishing) return@runOnUiThread
                    telegramLoginId = res.getString("loginId")
                    telegramSecret = res.getString("pollSecret")
                    telegramDeadline = SystemClock.elapsedRealtime() + res.optLong("expiresInS", 600) * 1000
                    info.setText(R.string.syria_login_telegram_waiting)
                    progress.visibility = View.VISIBLE
                    openTelegram(res.getString("botUrl"))
                }
            } catch (e: ApiException) {
                showError(
                    if (e.status == HTTP_UNAVAILABLE) getString(R.string.syria_login_telegram_unavailable) else e.message ?: e.toString(),
                )
            } catch (e: IOException) {
                Logger.w(TAG, "Telegram sign-in start failed: $e")
                showError(getString(R.string.syria_error_network))
            }
        }
    }

    /** t.me links open the Telegram app when installed, otherwise the browser page that leads to it. */
    private fun openTelegram(url: String) {
        try {
            startActivity(Intent(Intent.ACTION_VIEW, Uri.parse(url)))
        } catch (e: ActivityNotFoundException) {
            clearTelegram()
            showError(getString(R.string.syria_login_telegram_unavailable))
        }
    }

    private fun schedulePoll(delayMs: Long) {
        handler.removeCallbacks(pollRunnable)
        handler.postDelayed(pollRunnable, delayMs)
    }

    private fun pollTelegram() {
        val loginId = telegramLoginId ?: return
        val secret = telegramSecret ?: return
        if (SystemClock.elapsedRealtime() > telegramDeadline) {
            clearTelegram()
            showError(getString(R.string.syria_login_telegram_expired))
            return
        }
        executor.execute {
            try {
                val user = ApiClient.pollTelegram(this, loginId, secret)
                if (user != null) {
                    // Uploads what was queued before signing in (searches, map updates).
                    SyncWorker.enqueueNow(this)
                    runOnUiThread {
                        clearTelegram()
                        signedInWithoutPin(user)
                    }
                } else {
                    runOnUiThread { pollAgainIfShown() }
                }
            } catch (e: ApiException) {
                if (e.status == HTTP_NOT_FOUND || e.status == HTTP_GONE) {
                    runOnUiThread { clearTelegram() }
                    showError(getString(R.string.syria_login_telegram_expired))
                } else {
                    runOnUiThread { pollAgainIfShown() }
                }
            } catch (e: IOException) {
                Logger.w(TAG, "Telegram sign-in poll failed: $e")
                runOnUiThread { pollAgainIfShown() }
            }
        }
    }

    private fun pollAgainIfShown() {
        if (!isFinishing && lifecycle.currentState.isAtLeast(Lifecycle.State.RESUMED)) schedulePoll(POLL_MS)
    }

    private fun clearTelegram() {
        handler.removeCallbacks(pollRunnable)
        telegramLoginId = null
        telegramSecret = null
    }

    // ---- common

    /** After Telegram: an account without a PIN (new, or a forgotten one) is offered to set one. */
    private fun signedInWithoutPin(user: JSONObject) {
        if (isFinishing) return
        if (user.optBoolean("hasPin", true) || user.optString("role") == "admin") {
            finishSignedIn()
            return
        }
        setBusy(false)
        PinDialog.show(this) { finishSignedIn() }
    }

    private fun finishSignedIn() {
        if (isFinishing) return
        if (intent.getBooleanExtra(EXTRA_OPEN_TRACKING, true)) {
            val guide = intent.getBooleanExtra(TrackingActivity.EXTRA_DRIVING_GUIDE, false)
            val tracking = Intent(this, TrackingActivity::class.java)
            startActivity(tracking.putExtra(TrackingActivity.EXTRA_DRIVING_GUIDE, guide))
        }
        finish()
    }

    private fun showError(message: String) {
        runOnUiThread {
            if (isFinishing) return@runOnUiThread
            error.text = message
            setBusy(false)
        }
    }

    private fun setBusy(busy: Boolean) {
        if (busy) error.text = ""
        progress.visibility = if (busy) View.VISIBLE else View.GONE
        signInButton.isEnabled = !busy
        telegramButton.isEnabled = !busy
        anonymousButton.isEnabled = !busy
    }

    companion object {
        private const val TAG = "SyriaLogin"
        const val EXTRA_OPEN_TRACKING = "open_tracking"
        private const val MIN_PHONE_LENGTH = 7
        private const val PIN_LENGTH = 6
        private const val DEFAULT_LOCK_MINUTES = 15
        private const val POLL_MS = 2000L
        private const val HTTP_BAD_REQUEST = 400
        private const val HTTP_UNAUTHORIZED = 401
        private const val HTTP_NOT_FOUND = 404
        private const val HTTP_CONFLICT = 409
        private const val HTTP_GONE = 410
        private const val HTTP_TOO_MANY = 429
        private const val HTTP_UNAVAILABLE = 503
    }
}
