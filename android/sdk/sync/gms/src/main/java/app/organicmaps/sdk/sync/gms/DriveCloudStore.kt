package app.organicmaps.sdk.sync.gms

import app.organicmaps.sdk.sync.CloudException
import app.organicmaps.sdk.sync.CloudFile
import app.organicmaps.sdk.sync.CloudListing
import app.organicmaps.sdk.sync.CloudStore
import app.organicmaps.sdk.sync.SyncError
import java.io.File
import java.io.IOException
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.NonCancellable
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
    override suspend fun account(): String = user().getString("permissionId")

    suspend fun email(): String = user().getString("emailAddress")

    override suspend fun list(): CloudListing {
        val files = listFiles(
            "'$APP_DATA_FOLDER' in parents and mimeType != '$FOLDER_MIME_TYPE'",
            "id,name,md5Checksum,createdTime,appProperties",
        )
        val marker = files.firstOrNull { it.getString("name") == RESET_MARKER_NAME }
        return CloudListing(
            files.filter { it.getString("name").endsWith(KML_EXTENSION) }.map { it.toCloudFile() },
            marker?.let { it.optJSONObject("appProperties")?.optString(STARTED_AT)?.toLongOrNull() ?: 0L },
        )
    }

    override suspend fun currentMd5(id: String): String? {
        val file = callOrNull {
            get().url(endpoint("drive/v3/files/$id") { addQueryParameter("fields", "md5Checksum,appProperties") })
        }?.json() ?: return null
        val isTrashed = file.optJSONObject("appProperties")?.has(TRASHED_AT) == true
        return if (isTrashed) null else file.getString("md5Checksum")
    }

    override suspend fun download(id: String, dest: File) {
        call { get().url(endpoint("drive/v3/files/$id") { addQueryParameter("alt", "media") }) }.use { response ->
            try {
                withContext(io) { dest.outputStream().use { response.body.byteStream().copyTo(it) } }
            } catch (e: IOException) {
                throw CloudException(SyncError.Transient, e)
            }
        }
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
        val session = call {
            method(if (existingId == null) "POST" else "PATCH", metadata.toString().toRequestBody(JSON))
                .url(start)
                .header("X-Upload-Content-Type", KML_MIME_TYPE)
        }.use { it.header("Location") } ?: throw CloudException(SyncError.Other("No resumable upload session"))
        // The file is streamed from disk, so recorded tracks of any size never load into memory.
        return call { put(src.asRequestBody(KML_MIME_TYPE.toMediaType())).url(session) }.json().toCloudFile()
    }

    override suspend fun moveToTrash(id: String, trashFolderId: String?): String {
        val folderId = trashFolderId ?: findOrCreateTrashFolder()
        val properties = JSONObject().put("appProperties", JSONObject().put(TRASHED_AT, now().toString()))
        call {
            patch(properties.toString().toRequestBody(JSON)).url(
                endpoint("drive/v3/files/$id") {
                    addQueryParameter("addParents", folderId)
                    addQueryParameter("removeParents", APP_DATA_FOLDER)
                },
            )
        }.close()
        return folderId
    }

    override suspend fun purgeTrash(trashFolderId: String, trashedBefore: Long) {
        for (file in listFiles("'$trashFolderId' in parents", "id,appProperties")) {
            val trashedAt = file.optJSONObject("appProperties")?.optString(TRASHED_AT)?.toLongOrNull() ?: continue
            if (trashedAt < trashedBefore) delete(file.getString("id"))
        }
    }

    /**
     * Other devices see the marker and reconcile as after a reset instead of treating the files deleted so far as
     * deleted on purpose. The marker is created first and deleted last.
     */
    override suspend fun deleteAll() {
        val marker = JSONObject()
            .put("name", RESET_MARKER_NAME)
            .put("parents", JSONArray().put(APP_DATA_FOLDER))
            .put("appProperties", JSONObject().put(STARTED_AT, now().toString()))
        val markerId = call {
            post(marker.toString().toRequestBody(JSON)).url(
                endpoint("drive/v3/files") {
                    addQueryParameter("fields", "id")
                },
            )
        }.json().getString("id")
        try {
            for (file in listFiles("'$APP_DATA_FOLDER' in parents", "id")) {
                val id = file.getString("id")
                if (id != markerId) delete(id)
            }
        } finally {
            withContext(NonCancellable) { delete(markerId) }
        }
    }

    private suspend fun user(): JSONObject = call {
        get().url(endpoint("drive/v3/about") { addQueryParameter("fields", "user(permissionId,emailAddress)") })
    }
        .json()
        .getJSONObject("user")

    private suspend fun findOrCreateTrashFolder(): String {
        val query = "name = '$TRASH_FOLDER_NAME' and mimeType = '$FOLDER_MIME_TYPE' and '$APP_DATA_FOLDER' in parents"
        listFiles(query, "id").firstOrNull()?.let { return it.getString("id") }
        val folder = JSONObject()
            .put("name", TRASH_FOLDER_NAME)
            .put("mimeType", FOLDER_MIME_TYPE)
            .put("parents", JSONArray().put(APP_DATA_FOLDER))
        return call {
            post(folder.toString().toRequestBody(JSON)).url(
                endpoint("drive/v3/files") {
                    addQueryParameter("fields", "id")
                },
            )
        }.json().getString("id")
    }

    private suspend fun delete(id: String) {
        callOrNull { delete().url(endpoint("drive/v3/files/$id")) }?.close()
    }

    private suspend fun listFiles(query: String, fields: String): List<JSONObject> {
        val result = mutableListOf<JSONObject>()
        var pageToken: String? = null
        do {
            val page = call {
                get().url(
                    endpoint("drive/v3/files") {
                        addQueryParameter("spaces", APP_DATA_FOLDER)
                        addQueryParameter("q", query)
                        addQueryParameter("fields", "nextPageToken,files($fields)")
                        addQueryParameter("pageSize", PAGE_SIZE)
                        pageToken?.let { addQueryParameter("pageToken", it) }
                    },
                )
            }.json()
            val files = page.getJSONArray("files")
            for (i in 0 until files.length()) result += files.getJSONObject(i)
            pageToken = page.optString("nextPageToken").ifEmpty { null }
        } while (pageToken != null)
        return result
    }

    private fun endpoint(path: String, query: HttpUrl.Builder.() -> Unit = {}): HttpUrl =
        baseUrl.newBuilder().addPathSegments(path).apply(query).build()

    /** Runs the request; a successful response must be closed by the caller. */
    private suspend fun call(request: Request.Builder.() -> Request.Builder): Response {
        val response = send(request)
        if (response.code == HTTP_NOT_FOUND) throw response.toCloudException()
        return response
    }

    /** Like [call], but null when the file does not exist. */
    private suspend fun callOrNull(request: Request.Builder.() -> Request.Builder): Response? {
        val response = send(request)
        if (response.code != HTTP_NOT_FOUND) return response
        response.close()
        return null
    }

    /** Sends the request with a fresh token, retrying once with a new token after 401. Returns success or 404. */
    private suspend fun send(request: Request.Builder.() -> Request.Builder): Response = withContext(io) {
        var token = tokens.token()
        repeat(ATTEMPTS) { attempt ->
            val response = try {
                http.newCall(Request.Builder().request().header("Authorization", "Bearer $token").build()).execute()
            } catch (e: IOException) {
                throw CloudException(SyncError.Transient, e)
            }
            when {
                response.isSuccessful || response.code == HTTP_NOT_FOUND -> return@withContext response

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

    private fun Response.toCloudException() = CloudException(DriveErrors.map(code, use { it.body.string() }))

    private fun Response.json(): JSONObject = use {
        try {
            JSONObject(body.string())
        } catch (e: IOException) {
            throw CloudException(SyncError.Transient, e)
        }
    }

    private fun JSONObject.toCloudFile() =
        CloudFile(getString("id"), getString("name"), getString("md5Checksum"), getString("createdTime"))

    private companion object {
        const val DEFAULT_BASE_URL = "https://www.googleapis.com/"
        const val APP_DATA_FOLDER = "appDataFolder"
        const val FOLDER_MIME_TYPE = "application/vnd.google-apps.folder"
        const val KML_MIME_TYPE = "application/vnd.google-earth.kml+xml"
        const val KML_EXTENSION = ".kml"
        const val TRASH_FOLDER_NAME = ".Trash"
        const val TRASHED_AT = "trashedAt"
        const val RESET_MARKER_NAME = ".reset"
        const val STARTED_AT = "startedAt"
        const val FILE_FIELDS = "id,name,md5Checksum,createdTime"
        const val PAGE_SIZE = "1000"
        const val ATTEMPTS = 2
        const val HTTP_UNAUTHORIZED = 401
        const val HTTP_NOT_FOUND = 404
        val JSON = "application/json; charset=utf-8".toMediaType()
    }
}
