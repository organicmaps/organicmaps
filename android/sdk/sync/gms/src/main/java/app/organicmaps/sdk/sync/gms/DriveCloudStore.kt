package app.organicmaps.sdk.sync.gms

import androidx.annotation.Keep
import app.organicmaps.sdk.sync.CloudException
import app.organicmaps.sdk.sync.CloudFile
import app.organicmaps.sdk.sync.CloudStore
import app.organicmaps.sdk.sync.HashAlgorithm
import app.organicmaps.sdk.sync.StorageQuota
import app.organicmaps.sdk.sync.SyncError
import java.io.File
import java.io.IOException
import java.net.HttpURLConnection.HTTP_INTERNAL_ERROR
import java.net.HttpURLConnection.HTTP_NOT_FOUND
import java.net.HttpURLConnection.HTTP_UNAUTHORIZED
import java.time.Instant
import java.util.concurrent.ConcurrentHashMap
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withContext
import okhttp3.Call
import okhttp3.HttpUrl
import okhttp3.HttpUrl.Companion.toHttpUrl
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.RequestBody.Companion.asRequestBody
import okhttp3.RequestBody.Companion.toRequestBody
import okhttp3.Response
import org.json.JSONArray
import org.json.JSONObject

/** Category files in the hidden Google Drive appDataFolder, over Drive REST v3. Called by the core sync engine. */
@Keep
class DriveCloudStore(
    private val http: OkHttpClient,
    private val tokens: TokenProvider,
    private val baseUrl: HttpUrl = DEFAULT_BASE_URL.toHttpUrl(),
    private val io: CoroutineDispatcher = Dispatchers.IO,
    private val now: () -> Long = System::currentTimeMillis,
) : CloudStore {
    // Requests of the running pass, aborted by cancelPass(). Account requests of the settings screen are not.
    private val passCalls: MutableSet<Call> = ConcurrentHashMap.newKeySet()

    suspend fun email(): String =
        withContext(io) { about("user(emailAddress)").getJSONObject("user").getString("emailAddress") }

    suspend fun storageQuota(): StorageQuota = withContext(io) {
        // Drive sends 64-bit numbers as strings; there is no limit for unlimited storage.
        val quota = about("storageQuota(limit,usage)").getJSONObject("storageQuota")
        StorageQuota(quota.getString("usage").toLong(), quota.optString("limit").toLongOrNull())
    }

    override val hashAlgorithm = HashAlgorithm.MD5

    override fun list(): Array<CloudFile> =
        listFiles("'$APP_DATA_FOLDER' in parents and mimeType != '$FOLDER_MIME_TYPE'", FILE_FIELDS, isPass = true)
            .map { it.toCloudFile() }
            .toTypedArray()

    override fun stat(id: String): CloudFile? {
        val file = callOrNull(::json, isPass = true) {
            get().url(endpoint("drive/v3/files/$id") { addQueryParameter("fields", "$FILE_FIELDS,appProperties") })
        } ?: return null
        val isTrashed = file.optJSONObject("appProperties")?.has(TRASHED_AT) == true
        return if (isTrashed) null else file.toCloudFile()
    }

    override fun createGeneration(): String =
        create(JSONObject().put("name", GENERATION_NAME).put("parents", JSONArray().put(APP_DATA_FOLDER)))

    override fun download(id: String, destPath: String): Boolean {
        val copied = callOrNull({ response ->
            try {
                File(destPath).outputStream().use { response.body.byteStream().copyTo(it) }
            } catch (e: IOException) {
                throw CloudException(SyncError.Transient, e)
            }
        }, isPass = true) {
            get().url(endpoint("drive/v3/files/$id") { addQueryParameter("alt", "media") })
        }
        return copied != null
    }

    override fun upload(name: String, srcPath: String, existingId: String?): CloudFile {
        val metadata = if (existingId == null) {
            JSONObject().put("name", name).put("parents", JSONArray().put(APP_DATA_FOLDER))
        } else {
            JSONObject()
        }
        val start = endpoint(if (existingId == null) "upload/drive/v3/files" else "upload/drive/v3/files/$existingId") {
            addQueryParameter("uploadType", "resumable")
            addQueryParameter("fields", FILE_FIELDS)
        }
        val session = call({ it.header("Location") }) {
            method(if (existingId == null) "POST" else "PATCH", metadata.toString().toRequestBody(JSON))
                .url(start)
                .header("X-Upload-Content-Type", KML_MIME_TYPE)
        } ?: throw CloudException(SyncError.Other("No resumable upload session"))
        // The file is streamed from disk, so recorded tracks of any size never load into memory.
        return call(::json) { put(File(srcPath).asRequestBody(KML_MIME_TYPE.toMediaType())).url(session) }
            .toCloudFile()
    }

    override fun moveToTrash(id: String) {
        val folderId = findOrCreateTrashFolder()
        val properties = JSONObject().put("appProperties", JSONObject().put(TRASHED_AT, now().toString()))
        call({}) {
            patch(properties.toString().toRequestBody(JSON)).url(
                endpoint("drive/v3/files/$id") {
                    addQueryParameter("addParents", folderId)
                    addQueryParameter("removeParents", APP_DATA_FOLDER)
                },
            )
        }
    }

    // Every trash folder: devices that created one at the same time keep using theirs until they see the oldest one.
    override fun purgeTrash(trashedBeforeMs: Long) {
        for (folder in listTrashFolders()) {
            for (file in listFiles("'${folder.getString("id")}' in parents", "id,appProperties", isPass = true)) {
                val trashedAt = file.optJSONObject("appProperties")?.optString(TRASHED_AT)?.toLongOrNull() ?: continue
                if (trashedAt < trashedBeforeMs) delete(file.getString("id"))
            }
        }
    }

    override fun deleteAll(generationId: String?) {
        generationId?.let { delete(it) }
        val (generations, others) = listFiles("'$APP_DATA_FOLDER' in parents", "id,name", isPass = false)
            .partition { it.getString("name") == GENERATION_NAME }
        for (file in generations + others) delete(file.getString("id"))
    }

    override fun cancelPass() {
        passCalls.forEach { it.cancel() }
    }

    private fun about(fields: String): JSONObject =
        call(::json, isPass = false) { get().url(endpoint("drive/v3/about") { addQueryParameter("fields", fields) }) }

    private fun listTrashFolders(): List<JSONObject> = listFiles(
        "name = '$TRASH_FOLDER_NAME' and mimeType = '$FOLDER_MIME_TYPE' and '$APP_DATA_FOLDER' in parents",
        "id,createdTime",
        isPass = true,
    )

    private fun findOrCreateTrashFolder(): String {
        // The oldest one, so that devices agree on it.
        listTrashFolders().minWithOrNull(compareBy({ it.getString("createdTime") }, { it.getString("id") }))
            ?.let { return it.getString("id") }
        return create(
            JSONObject()
                .put("name", TRASH_FOLDER_NAME)
                .put("mimeType", FOLDER_MIME_TYPE)
                .put("parents", JSONArray().put(APP_DATA_FOLDER)),
        )
    }

    /** Creates a file without content. @return its id. */
    private fun create(metadata: JSONObject): String {
        val url = endpoint("drive/v3/files") { addQueryParameter("fields", "id") }
        return call(::json) { post(metadata.toString().toRequestBody(JSON)).url(url) }.getString("id")
    }

    private fun delete(id: String) {
        callOrNull({}, isPass = false) { delete().url(endpoint("drive/v3/files/$id")) }
    }

    private fun listFiles(query: String, fields: String, isPass: Boolean): List<JSONObject> {
        val result = mutableListOf<JSONObject>()
        var pageToken: String? = null
        do {
            val page = call(::json, isPass) {
                get().url(
                    endpoint("drive/v3/files") {
                        addQueryParameter("spaces", APP_DATA_FOLDER)
                        addQueryParameter("q", query)
                        addQueryParameter("fields", "nextPageToken,files($fields)")
                        addQueryParameter("pageSize", PAGE_SIZE)
                        pageToken?.let { addQueryParameter("pageToken", it) }
                    },
                )
            }
            val files = page.getJSONArray("files")
            for (i in 0 until files.length()) result += files.getJSONObject(i)
            pageToken = page.optString("nextPageToken").ifEmpty { null }
        } while (pageToken != null)
        return result
    }

    private fun endpoint(path: String, query: HttpUrl.Builder.() -> Unit = {}): HttpUrl =
        baseUrl.newBuilder().addPathSegments(path).apply(query).build()

    private fun <T> call(
        read: (Response) -> T,
        isPass: Boolean = true,
        request: Request.Builder.() -> Request.Builder,
    ): T {
        val response = send(isPass, request)
        if (response.code == HTTP_NOT_FOUND) throw response.toCloudException()
        return response.use(read)
    }

    /** Like [call], but null when the file does not exist. */
    private fun <T> callOrNull(
        read: (Response) -> T,
        isPass: Boolean = true,
        request: Request.Builder.() -> Request.Builder,
    ): T? {
        val response = send(isPass, request)
        if (response.code == HTTP_NOT_FOUND) {
            response.close()
            return null
        }
        return response.use(read)
    }

    /** Sends the request with a fresh token, retrying once with a new token after 401. Returns success or 404. */
    private fun send(isPass: Boolean, request: Request.Builder.() -> Request.Builder): Response {
        var token = runBlocking { tokens.token() }
        repeat(ATTEMPTS) { attempt ->
            val call = http.newCall(Request.Builder().request().header("Authorization", "Bearer $token").build())
            if (isPass) passCalls += call
            val response = try {
                call.execute()
            } catch (e: IOException) {
                throw CloudException(SyncError.Transient, e)
            } finally {
                passCalls -= call
            }
            when {
                response.isSuccessful || response.code == HTTP_NOT_FOUND -> return response

                response.code == HTTP_UNAUTHORIZED && attempt + 1 < ATTEMPTS -> {
                    response.close()
                    runBlocking { tokens.invalidate(token) }
                    token = runBlocking { tokens.token() }
                }

                else -> throw response.toCloudException()
            }
        }
        error("unreachable")
    }

    private fun Response.toCloudException(): CloudException {
        // The status code alone still classifies the error when the connection drops while its body is read.
        val body = use {
            try {
                it.body.string()
            } catch (ignored: IOException) {
                ""
            }
        }
        val error = runCatching { JSONObject(body).getJSONObject("error") }.getOrNull()
        val reason = error?.optJSONArray("errors")?.optJSONObject(0)?.optString("reason")
        val syncError = when {
            reason == "storageQuotaExceeded" -> SyncError.QuotaExceeded
            reason == "rateLimitExceeded" || reason == "userRateLimitExceeded" -> SyncError.Transient
            code == HTTP_UNAUTHORIZED -> SyncError.AuthRequired
            code == HTTP_TOO_MANY_REQUESTS || code >= HTTP_INTERNAL_ERROR -> SyncError.Transient
            else -> SyncError.Other(error?.optString("message").orEmpty().ifEmpty { "HTTP $code" })
        }
        return CloudException(syncError)
    }

    private fun json(response: Response): JSONObject = try {
        JSONObject(response.body.string())
    } catch (e: IOException) {
        throw CloudException(SyncError.Transient, e)
    }

    // A generation has no content, and so no MD5.
    private fun JSONObject.toCloudFile(): CloudFile {
        val md5 = optString("md5Checksum").ifEmpty { null }
        return CloudFile(
            id = getString("id"),
            name = getString("name"),
            version = md5.orEmpty(),
            hash = md5,
            createdTimeMs = Instant.parse(getString("createdTime")).toEpochMilli(),
        )
    }

    private companion object {
        const val DEFAULT_BASE_URL = "https://www.googleapis.com/"
        const val APP_DATA_FOLDER = "appDataFolder"
        const val FOLDER_MIME_TYPE = "application/vnd.google-apps.folder"
        const val KML_MIME_TYPE = "application/vnd.google-earth.kml+xml"
        const val TRASH_FOLDER_NAME = ".Trash"
        const val TRASHED_AT = "trashedAt"
        const val GENERATION_NAME = ".generation"
        const val FILE_FIELDS = "id,name,md5Checksum,createdTime"
        const val PAGE_SIZE = "1000"
        const val ATTEMPTS = 2

        // java.net.HttpURLConnection has no constant for it.
        const val HTTP_TOO_MANY_REQUESTS = 429

        val JSON = "application/json; charset=utf-8".toMediaType()
    }
}
