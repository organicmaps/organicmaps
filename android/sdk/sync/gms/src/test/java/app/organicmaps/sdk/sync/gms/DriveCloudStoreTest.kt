package app.organicmaps.sdk.sync.gms

import app.organicmaps.sdk.sync.CloudException
import app.organicmaps.sdk.sync.CloudFile
import app.organicmaps.sdk.sync.StorageQuota
import app.organicmaps.sdk.sync.SyncError
import java.io.File
import java.nio.file.Files
import java.time.Instant
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.runBlocking
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
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
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

    private fun file(name: String, md5: String, createdTime: String = "2026-10-01T10:00:00.000Z") =
        """{"id": "id-$name", "name": "$name", "md5Checksum": "$md5", "createdTime": "$createdTime"}"""

    private fun generation(id: String, createdTime: String) =
        """{"id": "$id", "name": ".generation", "createdTime": "$createdTime"}"""

    private fun tempFile(bytes: ByteArray) =
        Files.createTempFile("upload", ".kml").toFile().apply { writeBytes(bytes) }.path

    private fun tempPath() = Files.createTempFile("download", ".kml").toFile().path

    private fun cloudFile(name: String, md5: String) =
        CloudFile("id-$name", name, md5, md5, Instant.parse("2026-10-01T10:00:00.000Z").toEpochMilli())

    @Test
    fun list_followsPagesAndKeepsEveryFile() {
        val generation = generation("g", "2026-10-01T09:00:00.000Z")
        respond("""{"nextPageToken": "p2", "files": [${file("A.kml", "a")}, ${file("notes.txt", "n")}, $generation]}""")
        respond("""{"files": [${file("B.kml", "b")}]}""")

        val files = store.list()

        assertEquals(listOf("A.kml", "notes.txt", ".generation", "B.kml"), files.map { it.name })
        assertEquals(cloudFile("A.kml", "a"), files.first())
        assertEquals(
            CloudFile("g", ".generation", "", null, Instant.parse("2026-10-01T09:00:00.000Z").toEpochMilli()),
            files[2],
        )
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
    fun createGeneration_createsFileInAppData() {
        respond("""{"id": "g1"}""")

        assertEquals("g1", store.createGeneration())
        val create = take()
        assertEquals("POST", create.method)
        assertEquals("/drive/v3/files", create.url.encodedPath)
        assertEquals(".generation", create.json().getString("name"))
        assertEquals("appDataFolder", create.json().getJSONArray("parents").getString(0))
    }

    @Test
    fun deleteAll_deletesGenerationFirst() {
        respond(code = 204, body = "")
        val other = """{"id": "g2", "name": ".generation"}"""
        respond("""{"files": [{"id": "a", "name": "A.kml"}, $other, {"id": "t", "name": ".Trash"}]}""")
        repeat(3) { respond(code = 204, body = "") }

        store.deleteAll("g")

        assertEquals("/drive/v3/files/g", take().url.encodedPath)
        assertEquals("'appDataFolder' in parents", take().url.queryParameter("q"))
        assertEquals(
            listOf("/drive/v3/files/g2", "/drive/v3/files/a", "/drive/v3/files/t"),
            List(3) { take().url.encodedPath },
        )
    }

    @Test
    fun download_connectionDropDuringBody_throwsTransient() {
        server.enqueue(
            MockResponse.Builder().body("x".repeat(100_000)).onResponseBody(SocketEffect.CloseSocket()).build(),
        )
        assertEquals(SyncError.Transient, errorOf { store.download("id1", tempPath()) })
    }

    @Test
    fun download_missingFile_returnsFalse() {
        respond(code = 404, body = """{"error": {"code": 404, "message": "File not found"}}""")

        assertFalse(store.download("gone", tempPath()))
    }

    @Test
    fun request_errorBodyCutOff_mapsStatusCode() {
        server.enqueue(
            MockResponse.Builder().code(
                500,
            ).body("x".repeat(100_000)).onResponseBody(SocketEffect.CloseSocket()).build(),
        )

        assertEquals(SyncError.Transient, errorOf { store.list() })
    }

    @Test
    fun email_returnsAccountEmail() = runTest(dispatcher) {
        respond("""{"user": {"emailAddress": "user@example.com"}}""")

        assertEquals("user@example.com", store.email())
        val request = take()
        assertEquals("/drive/v3/about", request.url.encodedPath)
        assertEquals("user(emailAddress)", request.url.queryParameter("fields"))
    }

    @Test
    fun storageQuota_readsUsageAndLimit() = runTest(dispatcher) {
        respond("""{"storageQuota": {"limit": "2199023255552", "usage": "526889040000"}}""")

        assertEquals(StorageQuota(usage = 526_889_040_000, limit = 2_199_023_255_552), store.storageQuota())
        val request = take()
        assertEquals("/drive/v3/about", request.url.encodedPath)
        assertEquals("storageQuota(limit,usage)", request.url.queryParameter("fields"))
    }

    @Test
    fun storageQuota_unlimited_hasNoLimit() = runTest(dispatcher) {
        respond("""{"storageQuota": {"usage": "1024"}}""")

        assertEquals(StorageQuota(usage = 1024, limit = null), store.storageQuota())
    }

    @Test
    fun stat_returnsNullForMissingOrTrashedFile() {
        respond(code = 404, body = """{"error": {"code": 404, "message": "File not found"}}""")
        respond(
            """{"id": "id-A.kml", "name": "A.kml", "md5Checksum": "a", "createdTime": "2026-10-01T10:00:00.000Z",
            "appProperties": {"trashedAt": "1"}}""",
        )
        respond(file("A.kml", "a"))

        assertNull(store.stat("gone"))
        assertNull(store.stat("trashed"))
        assertEquals(cloudFile("A.kml", "a"), store.stat("id-A.kml"))
    }

    @Test
    fun download_writesBody() {
        respond("<kml/>")
        val dest = tempPath()

        assertTrue(store.download("id1", dest))

        assertEquals("<kml/>", File(dest).readText())
        val request = take()
        assertEquals("/drive/v3/files/id1", request.url.encodedPath)
        assertEquals("media", request.url.queryParameter("alt"))
    }

    @Test
    fun upload_create_startsResumableSessionWithAppDataParent() {
        respond(location = server.url("/session").toString())
        respond(file("A.kml", "a"))

        val result = store.upload("A.kml", tempFile("<kml/>".toByteArray()), null)

        assertEquals(cloudFile("A.kml", "a"), result)
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
    fun upload_update_patchesExistingId() {
        respond(location = server.url("/session").toString())
        respond(file("A.kml", "a2"))

        store.upload("A.kml", tempFile("<kml/>".toByteArray()), "id1")

        val start = take()
        assertEquals("PATCH", start.method)
        assertEquals("/upload/drive/v3/files/id1", start.url.encodedPath)
        assertEquals(0, start.json().length())
    }

    @Test
    fun upload_sendsUnicodeNameInMetadata() {
        respond(location = server.url("/session").toString())
        respond(file("Café ☕.kml", "c"))

        store.upload("Café ☕.kml", tempFile("<kml/>".toByteArray()), null)

        assertEquals("Café ☕.kml", take().json().getString("name"))
    }

    @Test
    fun upload_streamsFileLargerThanFiveMegabytes() {
        respond(location = server.url("/session").toString())
        respond(file("Track.kml", "t"))

        store.upload("Track.kml", tempFile(ByteArray(SIX_MEGABYTES) { 'a'.code.toByte() }), null)

        take()
        assertEquals(SIX_MEGABYTES.toLong(), take().bodySize)
        assertEquals(2, server.requestCount)
    }

    @Test
    fun moveToTrash_createsFolderOrUsesOldest_andSetsTrashedAt() {
        respond("""{"files": []}""")
        respond("""{"id": "trash1"}""")
        respond()
        val newer = """{"id": "trash2", "createdTime": "2026-10-02T10:00:00.000Z"}"""
        respond("""{"files": [$newer, {"id": "trash1", "createdTime": "2026-10-01T10:00:00.000Z"}]}""")
        respond()

        store.moveToTrash("id1")
        store.moveToTrash("id2")

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
        take()
        val secondMove = take()
        assertEquals("/drive/v3/files/id2", secondMove.url.encodedPath)
        assertEquals("trash1", secondMove.url.queryParameter("addParents"))
        assertEquals(5, server.requestCount)
    }

    @Test
    fun purgeTrash_deletesOnlyOlderThanThresholdInEveryTrashFolder() {
        respond("""{"files": [{"id": "trash1"}, {"id": "trash2"}]}""")
        respond(
            """{"files": [
                {"id": "old", "appProperties": {"trashedAt": "100"}},
                {"id": "recent", "appProperties": {"trashedAt": "300"}}
            ]}""",
        )
        respond(code = 204, body = "")
        respond("""{"files": [{"id": "old2", "appProperties": {"trashedAt": "150"}}]}""")
        respond(code = 204, body = "")

        store.purgeTrash(trashedBeforeMs = 200)

        take()
        assertEquals("'trash1' in parents", take().url.queryParameter("q"))
        val delete = take()
        assertEquals("DELETE", delete.method)
        assertEquals("/drive/v3/files/old", delete.url.encodedPath)
        assertEquals("'trash2' in parents", take().url.queryParameter("q"))
        assertEquals("/drive/v3/files/old2", take().url.encodedPath)
        assertEquals(5, server.requestCount)
    }

    @Test
    fun request_401_invalidatesTokenAndRetriesOnce() = runTest(dispatcher) {
        respond(code = 401, body = """{"error": {"code": 401, "message": "Invalid Credentials"}}""")
        respond("""{"user": {"emailAddress": "user@example.com"}}""")

        assertEquals("user@example.com", store.email())

        assertEquals(listOf("t1"), tokens.invalidated)
        take()
        assertEquals("Bearer t2", take().headers["Authorization"])
    }

    @Test
    fun request_401Twice_throwsAuthRequired() {
        respond(code = 401)
        respond(code = 401)

        assertEquals(SyncError.AuthRequired, errorOf { runBlocking { store.email() } })
    }

    @Test
    fun request_403Quota_throwsQuotaExceeded() {
        respond(
            code = 403,
            body = """{"error": {"code": 403, "message": "Full", "errors": [{"reason": "storageQuotaExceeded"}]}}""",
        )

        assertEquals(SyncError.QuotaExceeded, errorOf { store.upload("A.kml", tempFile(ByteArray(1)), null) })
    }

    @Test
    fun request_429_throwsTransient() {
        respond(code = 429)

        assertEquals(SyncError.Transient, errorOf { store.list() })
    }

    @Test
    fun request_otherClientError_throwsOtherWithMessage() {
        respond(code = 400, body = """{"error": {"code": 400, "message": "Invalid query"}}""")

        assertEquals(SyncError.Other("Invalid query"), errorOf { store.list() })
    }

    private fun errorOf(block: () -> Unit): SyncError {
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
