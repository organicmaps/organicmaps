package app.organicmaps.syria.account

import android.content.Intent
import android.os.Bundle
import android.view.View
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import androidx.appcompat.widget.Toolbar
import app.organicmaps.BuildConfig
import app.organicmaps.R
import app.organicmaps.base.BaseMwmFragmentActivity
import app.organicmaps.sdk.util.log.Logger
import app.organicmaps.syria.ApiClient
import app.organicmaps.syria.ApiException
import app.organicmaps.syria.tracking.TrackingActivity
import app.organicmaps.util.UiUtils
import app.organicmaps.util.Utils
import java.io.IOException
import java.util.concurrent.Executors

/** Phone number + SMS code login. Opens the tracking screen afterwards unless told otherwise. */
class LoginActivity : BaseMwmFragmentActivity() {
    private val executor = Executors.newSingleThreadExecutor()

    private lateinit var phoneInput: EditText
    private lateinit var codeInput: EditText
    private lateinit var sendButton: Button
    private lateinit var verifyButton: Button
    private lateinit var codeGroup: View
    private lateinit var progress: View
    private lateinit var error: TextView
    private lateinit var info: TextView
    private var phoneE164: String? = null

    override fun getContentLayoutResId(): Int = R.layout.activity_syria_login

    override fun onSafeCreate(savedInstanceState: Bundle?) {
        super.onSafeCreate(savedInstanceState)
        val toolbar = findViewById<Toolbar>(R.id.toolbar)
        toolbar.setTitle(R.string.syria_login_title)
        UiUtils.showHomeUpButton(toolbar)
        toolbar.setNavigationOnClickListener { finish() }

        phoneInput = findViewById(R.id.login_phone)
        codeInput = findViewById(R.id.login_code)
        sendButton = findViewById(R.id.login_send)
        verifyButton = findViewById(R.id.login_verify)
        codeGroup = findViewById(R.id.login_code_group)
        progress = findViewById(R.id.login_progress)
        error = findViewById(R.id.login_error)
        info = findViewById(R.id.login_info)

        if (phoneInput.text.isNullOrEmpty()) phoneInput.setText(DEFAULT_PREFIX)
        sendButton.setOnClickListener { sendCode() }
        verifyButton.setOnClickListener { verifyCode() }
        findViewById<View>(R.id.login_privacy).setOnClickListener { Utils.openUrl(this, BuildConfig.SYRIA_PRIVACY_URL) }
    }

    override fun onSafeDestroy() {
        executor.shutdownNow()
        super.onSafeDestroy()
    }

    private fun sendCode() {
        val phone = phoneInput.text.toString().trim()
        if (phone.length < MIN_PHONE_LENGTH) {
            error.setText(R.string.syria_login_invalid_phone)
            return
        }
        setBusy(true)
        executor.execute {
            try {
                val res = ApiClient.requestOtp(phone)
                val normalized = res.getString("phone")
                runOnUiThread {
                    phoneE164 = normalized
                    info.text = getString(R.string.syria_login_code_sent, normalized)
                    codeGroup.visibility = View.VISIBLE
                    codeInput.requestFocus()
                    setBusy(false)
                }
            } catch (e: ApiException) {
                showError(e.message ?: e.toString())
            } catch (e: IOException) {
                Logger.w(TAG, "OTP request failed: $e")
                showError(getString(R.string.syria_error_network))
            }
        }
    }

    private fun verifyCode() {
        val phone = phoneE164 ?: return
        val code = codeInput.text.toString().trim()
        if (code.isEmpty()) return
        setBusy(true)
        executor.execute {
            try {
                ApiClient.verifyOtp(this, phone, code)
                runOnUiThread {
                    if (intent.getBooleanExtra(EXTRA_OPEN_TRACKING, true)) {
                        startActivity(Intent(this, TrackingActivity::class.java))
                    }
                    finish()
                }
            } catch (e: ApiException) {
                showError(e.message ?: e.toString())
            } catch (e: IOException) {
                Logger.w(TAG, "OTP verification failed: $e")
                showError(getString(R.string.syria_error_network))
            }
        }
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
        sendButton.isEnabled = !busy
        verifyButton.isEnabled = !busy
    }

    companion object {
        private const val TAG = "SyriaLogin"
        const val EXTRA_OPEN_TRACKING = "open_tracking"
        private const val DEFAULT_PREFIX = "+963"
        private const val MIN_PHONE_LENGTH = 7
    }
}
