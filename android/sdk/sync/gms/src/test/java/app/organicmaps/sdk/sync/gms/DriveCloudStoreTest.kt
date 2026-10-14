package app.organicmaps.sdk.sync.gms

import app.organicmaps.sdk.sync.CloudException
import app.organicmaps.sdk.sync.CloudFile
import app.organicmaps.sdk.sync.SyncError
import java.io.File
import java.nio.file.Files
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.test.UnconfinedTestDispatcher
import kotlinx.coroutines.test.runTest
import mockwebserver3.MockResponse
import mockwebserver3.MockWebServer
import mockwebserver3.RecordedRequest
import mockwebserver3.SocketEffect
import okhttp3.OkHttpClient
import org.json.JSONObject
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.fail
import org.junit.Before
import org.junit.Test

@OptIn(ExperimentalCoroutinesApi::class)
class DriveCloudStoreTest {
    private val server = MockWebServer()
    private val tokens = FakeTokens()
    private val dispatcher = UnconfinedTestDispatcher()
    private lateinit var store: DriveCloudStore

    @Before
    fun setUp() {
        server.start()
        store = DriveCloudStore(OkHttpClient(), tokens, server.url("/"), dispatcher) { NOW }
    }

    @After
    fun tearDown() = server.close()

    private fun respond(body: String = "{}", code: Int = 200, location: String? = null) {
        val builder = MockResponse.Builder().code(code).body(body)
        location?.let { builder.addHeader("Location", it) }
        server.enqueue(builder.build())
    }

    private fun take(): RecordedRequest = server.takeRequest()

    private fun RecordedRequest.json() = JSONObject(body!!.utf8())

    private fun file(name: String, md5: String) =
        """{"id": "id-$name", "name": "$name", "md5Checksum": "$md5", "createdTime": "2026-10-01T10:00:00.000Z"}"""

    private fun tempFile(bytes: ByteArray) = Files.createTempFile("upload", ".kml").toFile().apply { writeBytes(bytes) }

    @Test
    fun list_followsPagesAndSkipsNonKml() = runTest(dispatcher) {
        respond("""{"nextPageToken": "p2", "files": [${file("A.kml", "a")}, ${file("notes.txt", "n")}]}""")
        respond("""{"files": [${file("B.kml", "b")}]}""")

        val files = store.list().files

        assertEquals(listOf("A.kml", "B.kml"), files.map { it.name })
        assertEquals(CloudFile("id-A.kml", "A.kml", "a", "2026-10-01T10:00:00.000Z"), files.first())
        val first = take()
        assertEquals("appDataFolder", first.url.queryParameter("spaces"))
        assertEquals(
            "'appDataFolder' in parents and mimeType != 'application/vnd.google-apps.folder'",
            first.url.queryParameter("q"),
        )
        assertEquals("Bearer t1", first.headers["Authorization"])
        assertEquals("p2", take().url.queryParameter("pageToken"))
    }

    @Test
    fun list_reportsResetMarker() = runTest(dispatcher) {
        respond(
            """{"files": [${file(
                "A.kml",
                "a",
            )}, {"id": "m", "name": ".reset", "appProperties": {"startedAt": "123"}}]}""",
        )

        val listing = store.list()

        assertEquals(listOf("A.kml"), listing.files.map { it.name })
        assertEquals(123L, listing.resetStartedAt)
    }

    @Test
    fun deleteAll_writesMarkerFirstAndDeletesItLast() = runTest(dispatcher) {
        respond("""{"id": "marker"}""")
        respond("""{"files": [{"id": "a"}, {"id": "marker"}, {"id": "trash"}]}""")
        respond(code = 204, body = "")
        respond(code = 204, body = "")
        respond(code = 204, body = "")

        store.deleteAll()

        val create = take()
        assertEquals("POST", create.method)
        assertEquals(".reset", create.json().getString("name"))
        assertEquals(NOW.toString(), create.json().getJSONObject("appProperties").getString("startedAt"))
        take()
        assertEquals(
            listOf("/drive/v3/files/a", "/drive/v3/files/trash", "/drive/v3/files/marker"),
            List(3) {
                take().url.encodedPath
            },
        )
    }

    @Test
    fun download_connectionDropDuringBody_throwsTransient() = runTest(dispatcher) {
        server.enqueue(
            MockResponse.Builder().body("x".repeat(100_000)).onResponseBody(SocketEffect.CloseSocket()).build(),
        )
        val dest = Files.createTempFile("download", ".part").toFile()

        assertEquals(SyncError.Transient, errorOf { store.download("id1", dest) })
    }

    @Test
    fun account_returnsPermissionId() = runTest(dispatcher) {
        respond("""{"user": {"permissionId": "perm-1", "emailAddress": "user@example.com"}}""")

        assertEquals("perm-1", store.account())
        assertEquals("/drive/v3/about", take().url.encodedPath)
    }

    @Test
    fun currentMd5_returnsNullForMissingOrTrashedFile() = runTest(dispatcher) {
        respond(code = 404, body = """{"error": {"code": 404, "message": "File not found"}}""")
        respond("""{"md5Checksum": "x", "appProperties": {"trashedAt": "1"}}""")
        respond("""{"md5Checksum": "x"}""")

        assertNull(store.currentMd5("gone"))
        assertNull(store.currentMd5("trashed"))
        assertEquals("x", store.currentMd5("live"))
    }

    @Test
    fun download_writesBody() = runTest(dispatcher) {
        respond("<kml/>")
        val dest = Files.createTempFile("download", ".part").toFile()

        store.download("id1", dest)

        assertEquals("<kml/>", dest.readText())
        val request = take()
        assertEquals("/drive/v3/files/id1", request.url.encodedPath)
        assertEquals("media", request.url.queryParameter("alt"))
    }

    @Test
    fun upload_create_startsResumableSessionWithAppDataParent() = runTest(dispatcher) {
        respond(location = server.url("/session").toString())
        respond(file("A.kml", "a"))

        val result = store.upload("A.kml", tempFile("<kml/>".toByteArray()), null)

        assertEquals(CloudFile("id-A.kml", "A.kml", "a", "2026-10-01T10:00:00.000Z"), result)
        val start = take()
        assertEquals("POST", start.method)
        assertEquals("/upload/drive/v3/files", start.url.encodedPath)
        assertEquals("resumable", start.url.queryParameter("uploadType"))
        assertEquals("application/vnd.google-earth.kml+xml", start.headers["X-Upload-Content-Type"])
        assertEquals("appDataFolder", start.json().getJSONArray("parents").getString(0))
        val put = take()
        assertEquals("PUT", put.method)
        assertEquals("/session", put.url.encodedPath)
        assertEquals("<kml/>", put.body!!.utf8())
    }

    @Test
    fun upload_update_patchesExistingId() = runTest(dispatcher) {
        respond(location = server.url("/session").toString())
        respond(file("A.kml", "a2"))

        store.upload("A.kml", tempFile("<kml/>".toByteArray()), "id1")

        val start = take()
        assertEquals("PATCH", start.method)
        assertEquals("/upload/drive/v3/files/id1", start.url.encodedPath)
        assertEquals(0, start.json().length())
    }

    @Test
    fun upload_sendsUnicodeNameInMetadata() = runTest(dispatcher) {
        respond(location = server.url("/session").toString())
        respond(file("Café ☕.kml", "c"))

        store.upload("Café ☕.kml", tempFile("<kml/>".toByteArray()), null)

        assertEquals("Café ☕.kml", take().json().getString("name"))
    }

    @Test
    fun upload_streamsFileLargerThanFiveMegabytes() = runTest(dispatcher) {
        respond(location = server.url("/session").toString())
        respond(file("Track.kml", "t"))

        store.upload("Track.kml", tempFile(ByteArray(SIX_MEGABYTES) { 'a'.code.toByte() }), null)

        take()
        assertEquals(SIX_MEGABYTES.toLong(), take().bodySize)
        assertEquals(2, server.requestCount)
    }

    @Test
    fun moveToTrash_createsFolderOnce_andSetsTrashedAt() = runTest(dispatcher) {
        respond("""{"files": []}""")
        respond("""{"id": "trash1"}""")
        respond()
        respond()

        assertEquals("trash1", store.moveToTrash("id1", null))
        assertEquals("trash1", store.moveToTrash("id2", "trash1"))

        assertEquals(
            "name = '.Trash' and mimeType = 'application/vnd.google-apps.folder' and 'appDataFolder' in parents",
            take().url.queryParameter("q"),
        )
        val create = take()
        assertEquals(".Trash", create.json().getString("name"))
        assertEquals("application/vnd.google-apps.folder", create.json().getString("mimeType"))
        val move = take()
        assertEquals("PATCH", move.method)
        assertEquals("/drive/v3/files/id1", move.url.encodedPath)
        assertEquals("trash1", move.url.queryParameter("addParents"))
        assertEquals("appDataFolder", move.url.queryParameter("removeParents"))
        assertEquals(NOW.toString(), move.json().getJSONObject("appProperties").getString("trashedAt"))
        assertEquals("/drive/v3/files/id2", take().url.encodedPath)
        assertEquals(4, server.requestCount)
    }

    @Test
    fun purgeTrash_deletesOnlyOlderThanThreshold() = runTest(dispatcher) {
        respond(
            """{"files": [
                {"id": "old", "appProperties": {"trashedAt": "100"}},
                {"id": "recent", "appProperties": {"trashedAt": "300"}}
            ]}""",
        )
        respond(code = 204, body = "")

        store.purgeTrash("trash1", trashedBefore = 200)

        assertEquals("'trash1' in parents", take().url.queryParameter("q"))
        val delete = take()
        assertEquals("DELETE", delete.method)
        assertEquals("/drive/v3/files/old", delete.url.encodedPath)
        assertEquals(2, server.requestCount)
    }

    @Test
    fun request_401_invalidatesTokenAndRetriesOnce() = runTest(dispatcher) {
        respond(code = 401, body = """{"error": {"code": 401, "message": "Invalid Credentials"}}""")
        respond("""{"user": {"permissionId": "perm-1"}}""")

        assertEquals("perm-1", store.account())

        assertEquals(listOf("t1"), tokens.invalidated)
        take()
        assertEquals("Bearer t2", take().headers["Authorization"])
    }

    @Test
    fun request_401Twice_throwsAuthRequired() = runTest(dispatcher) {
        respond(code = 401)
        respond(code = 401)

        assertEquals(SyncError.AuthRequired, errorOf { store.account() })
    }

    @Test
    fun request_403Quota_throwsQuotaExceeded() = runTest(dispatcher) {
        respond(
            code = 403,
            body = """{"error": {"code": 403, "message": "Full", "errors": [{"reason": "storageQuotaExceeded"}]}}""",
        )

        assertEquals(SyncError.QuotaExceeded, errorOf { store.upload("A.kml", tempFile(ByteArray(1)), null) })
    }

    @Test
    fun request_429_throwsTransient() = runTest(dispatcher) {
        respond(code = 429)

        assertEquals(SyncError.Transient, errorOf { store.list() })
    }

    @Test
    fun request_otherClientError_throwsOtherWithMessage() = runTest(dispatcher) {
        respond(code = 400, body = """{"error": {"code": 400, "message": "Invalid query"}}""")

        assertEquals(SyncError.Other("Invalid query"), errorOf { store.list() })
    }

    private suspend fun errorOf(block: suspend () -> Unit): SyncError {
        try {
            block()
        } catch (e: CloudException) {
            return e.error
        }
        fail("CloudException expected")
        error("unreachable")
    }

    private class FakeTokens : TokenProvider {
        private var generation = 1
        val invalidated = mutableListOf<String>()

        override suspend fun token() = "t$generation"

        override suspend fun invalidate(token: String) {
            invalidated += token
            ++generation
        }
    }

    private companion object {
        const val NOW = 1_800_000_000_000L
        const val SIX_MEGABYTES = 6 * 1024 * 1024
    }
}
