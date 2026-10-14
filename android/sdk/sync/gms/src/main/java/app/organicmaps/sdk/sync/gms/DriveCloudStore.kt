package app.organicmaps.sdk.sync.gms

import app.organicmaps.sdk.sync.CloudException
import app.organicmaps.sdk.sync.CloudFile
import app.organicmaps.sdk.sync.CloudListing
import app.organicmaps.sdk.sync.CloudStore
import app.organicmaps.sdk.sync.FileNames
import app.organicmaps.sdk.sync.StorageQuota
import app.organicmaps.sdk.sync.SyncError
import java.io.File
import java.io.IOException
import java.net.HttpURLConnection.HTTP_NOT_FOUND
import java.net.HttpURLConnection.HTTP_UNAUTHORIZED
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
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

/** Category files in the hidden Google Drive appDataFolder, over Drive REST v3. */
class DriveCloudStore(
    private val http: OkHttpClient,
    private val tokens: TokenProvider,
    private val baseUrl: HttpUrl = DEFAULT_BASE_URL.toHttpUrl(),
    private val io: CoroutineDispatcher = Dispatchers.IO,
    private val now: () -> Long = System::currentTimeMillis,
) : CloudStore {
    suspend fun email(): String = about("user(emailAddress)").getJSONObject("user").getString("emailAddress")

    suspend fun storageQuota(): StorageQuota {
        // Drive sends 64-bit numbers as strings; there is no limit for unlimited storage.
        val quota = about("storageQuota(limit,usage)").getJSONObject("storageQuota")
        return StorageQuota(quota.getString("usage").toLong(), quota.optString("limit").toLongOrNull())
    }

    override suspend fun list(): CloudListing {
        val files = listFiles(
            "'$APP_DATA_FOLDER' in parents and mimeType != '$FOLDER_MIME_TYPE'",
            "id,name,md5Checksum,createdTime",
        )
        // Devices that created a generation at the same time agree on the oldest one.
        val generation = files.filter { it.getString("name") == GENERATION_NAME }
            .minWithOrNull(compareBy({ it.getString("createdTime") }, { it.getString("id") }))
            ?: return CloudListing(emptyList(), null)
        // Older files are left from deleted data, which another device may still be deleting one by one.
        val since = generation.getString("createdTime")
        val kmlFiles = files.filter {
            it.getString("name").endsWith(FileNames.KML_EXTENSION) && it.getString("createdTime") >= since
        }
        return CloudListing(kmlFiles.map { it.toCloudFile() }, generation.getString("id"))
    }

    override suspend fun createGeneration(): String =
        create(JSONObject().put("name", GENERATION_NAME).put("parents", JSONArray().put(APP_DATA_FOLDER)))

    override suspend fun currentMd5(id: String): String? {
        val file = callOrNull(::json) {
            get().url(endpoint("drive/v3/files/$id") { addQueryParameter("fields", "md5Checksum,appProperties") })
        } ?: return null
        val isTrashed = file.optJSONObject("appProperties")?.has(TRASHED_AT) == true
        return if (isTrashed) null else file.getString("md5Checksum")
    }

    override suspend fun download(id: String, dest: File): Boolean {
        val copied = callOrNull({ response ->
            try {
                dest.outputStream().use { response.body.byteStream().copyTo(it) }
            } catch (e: IOException) {
                throw CloudException(SyncError.Transient, e)
            }
        }) {
            get().url(endpoint("drive/v3/files/$id") { addQueryParameter("alt", "media") })
        }
        return copied != null
    }

    override suspend fun upload(name: String, src: File, existingId: String?): CloudFile {
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
        return call(::json) { put(src.asRequestBody(KML_MIME_TYPE.toMediaType())).url(session) }.toCloudFile()
    }

    override suspend fun moveToTrash(id: String, trashFolderId: String?): String {
        val folderId = trashFolderId ?: findOrCreateTrashFolder()
        val properties = JSONObject().put("appProperties", JSONObject().put(TRASHED_AT, now().toString()))
        call({}) {
            patch(properties.toString().toRequestBody(JSON)).url(
                endpoint("drive/v3/files/$id") {
                    addQueryParameter("addParents", folderId)
                    addQueryParameter("removeParents", APP_DATA_FOLDER)
                },
            )
        }
        return folderId
    }

    override suspend fun purgeTrash(trashFolderId: String, trashedBefore: Long) {
        for (file in listFiles("'$trashFolderId' in parents", "id,appProperties")) {
            val trashedAt = file.optJSONObject("appProperties")?.optString(TRASHED_AT)?.toLongOrNull() ?: continue
            if (trashedAt < trashedBefore) delete(file.getString("id"))
        }
    }

    private suspend fun about(fields: String): JSONObject =
        call(::json) { get().url(endpoint("drive/v3/about") { addQueryParameter("fields", fields) }) }

    override suspend fun deleteAll() {
        val (generations, others) = listFiles("'$APP_DATA_FOLDER' in parents", "id,name")
            .partition { it.getString("name") == GENERATION_NAME }
        for (file in generations + others) delete(file.getString("id"))
    }

    private suspend fun findOrCreateTrashFolder(): String {
        val query = "name = '$TRASH_FOLDER_NAME' and mimeType = '$FOLDER_MIME_TYPE' and '$APP_DATA_FOLDER' in parents"
        listFiles(query, "id").firstOrNull()?.let { return it.getString("id") }
        return create(
            JSONObject()
                .put("name", TRASH_FOLDER_NAME)
                .put("mimeType", FOLDER_MIME_TYPE)
                .put("parents", JSONArray().put(APP_DATA_FOLDER)),
        )
    }

    /** Creates a file without content. @return its id. */
    private suspend fun create(metadata: JSONObject): String {
        val url = endpoint("drive/v3/files") { addQueryParameter("fields", "id") }
        return call(::json) { post(metadata.toString().toRequestBody(JSON)).url(url) }.getString("id")
    }

    private suspend fun delete(id: String) {
        callOrNull({}) { delete().url(endpoint("drive/v3/files/$id")) }
    }

    private suspend fun listFiles(query: String, fields: String): List<JSONObject> {
        val result = mutableListOf<JSONObject>()
        var pageToken: String? = null
        do {
            val page = call(::json) {
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

    /**
     * Runs the request and [read]s the successful response, both on [io]: reading or closing a response can use the
     * network too, which Android forbids on the main thread.
     */
    private suspend fun <T> call(read: (Response) -> T, request: Request.Builder.() -> Request.Builder): T =
        withContext(io) {
            val response = send(request)
            if (response.code == HTTP_NOT_FOUND) throw response.toCloudException()
            response.use(read)
        }

    /** Like [call], but null when the file does not exist. */
    private suspend fun <T> callOrNull(read: (Response) -> T, request: Request.Builder.() -> Request.Builder): T? =
        withContext(io) {
            val response = send(request)
            if (response.code == HTTP_NOT_FOUND) {
                response.close()
                null
            } else {
                response.use(read)
            }
        }

    /** Sends the request with a fresh token, retrying once with a new token after 401. Returns success or 404. */
    private suspend fun send(request: Request.Builder.() -> Request.Builder): Response {
        var token = tokens.token()
        repeat(ATTEMPTS) { attempt ->
            val response = try {
                http.newCall(Request.Builder().request().header("Authorization", "Bearer $token").build()).execute()
            } catch (e: IOException) {
                throw CloudException(SyncError.Transient, e)
            }
            when {
                response.isSuccessful || response.code == HTTP_NOT_FOUND -> return response

                response.code == HTTP_UNAUTHORIZED && attempt + 1 < ATTEMPTS -> {
                    response.close()
                    tokens.invalidate(token)
                    token = tokens.token()
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
        return CloudException(DriveErrors.map(code, body))
    }

    private fun json(response: Response): JSONObject = try {
        JSONObject(response.body.string())
    } catch (e: IOException) {
        throw CloudException(SyncError.Transient, e)
    }

    private fun JSONObject.toCloudFile() =
        CloudFile(getString("id"), getString("name"), getString("md5Checksum"), getString("createdTime"))

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
        val JSON = "application/json; charset=utf-8".toMediaType()
    }
}
