package app.organicmaps.syria

import android.content.Context
import android.os.Build
import androidx.annotation.WorkerThread
import app.organicmaps.BuildConfig
import java.io.IOException
import java.util.concurrent.TimeUnit
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.RequestBody.Companion.toRequestBody
import org.json.JSONArray
import org.json.JSONObject

/** Error answered by the tracking API (HTTP status + server message). */
class ApiException(val status: Int, message: String) : IOException(message)

/** Blocking client for the Syria Organic Maps tracking API. Call from a worker thread. */
object ApiClient {
    private val JSON = "application/json; charset=utf-8".toMediaType()

    private val client: OkHttpClient by lazy {
        OkHttpClient.Builder()
            .connectTimeout(15, TimeUnit.SECONDS)
            .readTimeout(60, TimeUnit.SECONDS)
            .writeTimeout(60, TimeUnit.SECONDS)
            .build()
    }

    val baseUrl: String
        get() = SyriaServer.apiUrl

    @JvmStatic
    fun deviceJson(context: Context): JSONObject =
        JSONObject()
            .put("id", SyriaPrefs.deviceId(context))
            .put("flavor", BuildConfig.FLAVOR)
            .put("model", Build.MODEL)
            .put("osVersion", Build.VERSION.RELEASE)
            .put("appVersion", BuildConfig.VERSION_NAME)

    /** Starts a Telegram sign-in: open `botUrl`, then call [pollTelegram] with `loginId` and `pollSecret`. */
    @WorkerThread
    fun startTelegram(): JSONObject = post("/auth/telegram/start", JSONObject(), null)

    /**
     * The signed-in user (`hasPin`, `phone`, ...) once the user shared their number with the bot, the session
     * is stored then; null while waiting.
     */
    @WorkerThread
    fun pollTelegram(context: Context, loginId: String, pollSecret: String): JSONObject? {
        val body = JSONObject()
            .put("loginId", loginId)
            .put("pollSecret", pollSecret)
            .put("device", deviceJson(context))
        val res = post("/auth/telegram/poll", body, null)
        if (res.optString("status") != "verified") return null
        storeSession(context, res, null)
        return res.getJSONObject("user")
    }

    /**
     * Phone number + 6-digit PIN. Registers a new number right away (`created` is true then), otherwise
     * checks the PIN. Throws ApiException 401 (wrong PIN), 409 (number without a PIN yet), 429 (locked).
     */
    @WorkerThread
    fun pinLogin(context: Context, phone: String, pin: String): JSONObject {
        val body = JSONObject().put("phone", phone).put("pin", pin).put("device", deviceJson(context))
        val res = post("/auth/pin/login", body, null)
        storeSession(context, res, phone)
        return res
    }

    /**
     * Anonymous recording: signs this install in without a phone number. Its recordings and live location
     * are kept under the device id; the same account comes back after signing out and in again.
     */
    @WorkerThread
    fun deviceLogin(context: Context): JSONObject {
        val body = JSONObject().put("secret", SyriaPrefs.deviceSecret(context)).put("device", deviceJson(context))
        val res = post("/auth/device", body, null)
        storeSession(context, res, null)
        return res
    }

    /** Sets or changes the PIN of the signed-in user. */
    @WorkerThread
    fun setPin(context: Context, pin: String): JSONObject = authed(context) { token ->
        put("/me/pin", JSONObject().put("pin", pin), token)
    }

    private fun storeSession(context: Context, res: JSONObject, fallbackPhone: String?) {
        val user = res.getJSONObject("user")
        val name = user.optString("displayName").takeIf { it.isNotEmpty() && it != "null" }
        val session = SyriaPrefs.Session(
            accessToken = res.getString("accessToken"),
            refreshToken = res.getString("refreshToken"),
            userId = user.getString("id"),
            phone = user.optString("phone").takeIf { it.isNotEmpty() && it != "null" } ?: fallbackPhone,
            displayName = name,
            anonymous = user.optBoolean("anonymous"),
        )
        SyriaPrefs.setSession(context, session)
    }

    /** Rotates the tokens. Returns false and clears the session when the refresh token was rejected. */
    @WorkerThread
    fun refresh(context: Context): Boolean {
        val refreshToken = SyriaPrefs.refreshToken(context) ?: return false
        return try {
            val res = post("/auth/refresh", JSONObject().put("refreshToken", refreshToken), null)
            SyriaPrefs.setTokens(context, res.getString("accessToken"), res.getString("refreshToken"))
            true
        } catch (e: ApiException) {
            if (e.status == 401 || e.status == 403) SyriaPrefs.clearSession(context)
            false
        }
    }

    /** Clears the local session first, so logging out works offline; the server call is best effort. */
    @WorkerThread
    fun logout(context: Context) {
        val refreshToken = SyriaPrefs.refreshToken(context)
        SyriaPrefs.clearSession(context)
        if (refreshToken != null) {
            runCatching { post("/auth/logout", JSONObject().put("refreshToken", refreshToken), null) }
        }
    }

    /** Live location points (provider "live" on the server); not queued when this fails. */
    @WorkerThread
    fun sendLivePoints(context: Context, points: List<JSONObject>): JSONObject = authed(context) { token ->
        post("/live/points", JSONObject().put("points", JSONArray(points)), token)
    }

    @WorkerThread
    fun syncBatch(context: Context, batch: JSONObject): JSONObject = authed(context) { token ->
        post("/sync/batch", batch, token)
    }

    private fun <T> authed(context: Context, call: (String) -> T): T {
        val token = SyriaPrefs.accessToken(context) ?: throw ApiException(401, "Not logged in")
        return try {
            call(token)
        } catch (e: ApiException) {
            // One transparent retry with a refreshed access token on 401.
            val fresh = if (e.status == 401 && refresh(context)) SyriaPrefs.accessToken(context) else null
            call(fresh ?: throw e)
        }
    }

    private fun put(path: String, body: JSONObject, token: String?): JSONObject {
        val builder = Request.Builder().url(baseUrl + path).put(body.toString().toRequestBody(JSON))
        return execute(builder, token)
    }

    private fun post(path: String, body: JSONObject, token: String?): JSONObject {
        val builder = Request.Builder().url(baseUrl + path).post(body.toString().toRequestBody(JSON))
        return execute(builder, token)
    }

    private fun execute(builder: Request.Builder, token: String?): JSONObject {
        token?.let { builder.header("Authorization", "Bearer $it") }
        client.newCall(builder.build()).execute().use { response ->
            val text = response.body.string()
            if (!response.isSuccessful) {
                val message = runCatching { JSONObject(text).getJSONObject("error").getString("message") }
                    .getOrDefault(response.message.ifEmpty { "HTTP ${response.code}" })
                throw ApiException(response.code, message)
            }
            return if (text.isBlank()) JSONObject() else JSONObject(text)
        }
    }
}
