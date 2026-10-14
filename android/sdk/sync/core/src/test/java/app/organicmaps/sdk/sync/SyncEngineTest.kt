package app.organicmaps.sdk.sync

import app.organicmaps.sdk.sync.testing.FakeCloudStore
import app.organicmaps.sdk.sync.testing.FakeCoreBookmarks
import java.io.File
import java.nio.file.Files
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.joinAll
import kotlinx.coroutines.launch
import kotlinx.coroutines.test.UnconfinedTestDispatcher
import kotlinx.coroutines.test.runTest
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test

@OptIn(ExperimentalCoroutinesApi::class)
class SyncEngineTest {
    private var now = 1_800_000_000_000L
    private val dispatcher = UnconfinedTestDispatcher()
    private val core = FakeCoreBookmarks()
    private val cloud = FakeCloudStore(clock = { now })
    private lateinit var dir: File
    private lateinit var tmpDir: File
    private lateinit var baseStore: BaseStateStore
    private lateinit var engine: SyncEngine

    @Before
    fun setUp() {
        val root = Files.createTempDirectory("sync").toFile()
        dir = File(root, "bookmarks").apply { mkdirs() }
        tmpDir = File(root, "sync/tmp")
        baseStore = BaseStateStore(File(root, "sync/google_drive.json"))
        val local = LocalStore(dir, tmpDir, core, dispatcher, dispatcher)
        engine = SyncEngine(cloud, local, baseStore, "Pixel 6") { now }
    }

    private fun write(name: String, text: String) = File(dir, name).writeText(text)

    private fun read(name: String) = File(dir, name).readText()

    private fun localNames() = dir.list()!!.sorted()

    @Test
    fun runPass_initialSync_uploadsLocalAndDownloadsCloud() = runTest(dispatcher) {
        write("A.kml", "a")
        cloud.put("B.kml", "b")

        assertEquals(PassResult.Synced, engine.runPass())

        assertEquals(listOf("A.kml", "B.kml"), cloud.names())
        assertEquals("a", cloud.content("A.kml"))
        assertEquals("b", read("B.kml"))
        assertEquals(setOf("A.kml", "B.kml"), baseStore.load().files.keys)
        assertEquals(SyncStatus.Idle(now), engine.status.value)
    }

    @Test
    fun runPass_secondPassIsIdle() = runTest(dispatcher) {
        write("A.kml", "a")
        cloud.put("B.kml", "b")
        engine.runPass()
        cloud.writes = 0
        val reloads = core.reloaded.size

        assertEquals(PassResult.Synced, engine.runPass())

        assertEquals(0, cloud.writes)
        assertEquals(reloads, core.reloaded.size)
    }

    @Test
    fun runPass_conflict_keepsCopyAndTakesCloud() = runTest(dispatcher) {
        write("A.kml", "<kml><Placemark>v1</Placemark></kml>")
        core.names["A.kml"] = "A"
        engine.runPass()
        write("A.kml", "<kml><Placemark>local v2</Placemark></kml>")
        cloud.files.values.single().content = "<kml><Placemark>cloud v2</Placemark></kml>".toByteArray()

        engine.runPass()

        assertEquals("<kml><Placemark>cloud v2</Placemark></kml>", read("A.kml"))
        assertEquals("<kml><Placemark>local v2</Placemark></kml>", read("A (Pixel 6).kml"))
        assertEquals("A (Pixel 6)", core.names["A (Pixel 6).kml"])
        assertEquals(
            FakeCloudStore.md5("<kml><Placemark>cloud v2</Placemark></kml>".toByteArray()),
            baseStore.load().files.getValue("A.kml").md5,
        )
    }

    @Test
    fun runPass_conflictWithListEmptiedElsewhere_keepsBothVersions() = runTest(dispatcher) {
        write("A.kml", "<kml><Placemark>v1</Placemark></kml>")
        core.names["A.kml"] = "A"
        engine.runPass()
        write("A.kml", "<kml><Placemark>local v2</Placemark></kml>")
        cloud.files.values.single().content = "<kml><Document><name>A</name></Document></kml>".toByteArray()

        engine.runPass()

        assertEquals("<kml><Document><name>A</name></Document></kml>", read("A.kml"))
        assertEquals("<kml><Placemark>local v2</Placemark></kml>", read("A (Pixel 6).kml"))
    }

    @Test
    fun runPass_initialSyncWithEmptyLocalList_takesCloudWithoutCopy() = runTest(dispatcher) {
        write("My Places.kml", "<kml><Document><name>My Places</name></Document></kml>")
        cloud.put("My Places.kml", "<kml><Placemark>theirs</Placemark></kml>")

        engine.runPass()

        assertEquals(listOf("My Places.kml"), localNames())
        assertEquals("<kml><Placemark>theirs</Placemark></kml>", read("My Places.kml"))
        assertEquals(listOf("My Places.kml"), cloud.names())
    }

    @Test
    fun runPass_listRecreatedElsewhereAfterDeletion_replacesItWithoutCopy() = runTest(dispatcher) {
        write("A.kml", "<kml><Placemark>old</Placemark></kml>")
        engine.runPass()
        // Another device deleted the list and then created a new one with the same name.
        cloud.files.values.single().trashedAt = now
        val recreated = cloud.put("A.kml", "<kml><Placemark>new</Placemark></kml>")

        engine.runPass()

        assertEquals(listOf("A.kml"), localNames())
        assertEquals("<kml><Placemark>new</Placemark></kml>", read("A.kml"))
        assertEquals(recreated, baseStore.load().files.getValue("A.kml").cloudId)
    }

    @Test
    fun runPass_otherFileTakesNameWhileSyncedOneIsUnlisted_waits() = runTest(dispatcher) {
        write("A.kml", "<kml><Placemark>old</Placemark></kml>")
        engine.runPass()
        val synced = cloud.files.keys.single()
        cloud.hiddenIdsFromListing += synced
        cloud.put("A.kml", "<kml><Placemark>other</Placemark></kml>")

        engine.runPass()

        assertEquals("<kml><Placemark>old</Placemark></kml>", read("A.kml"))
        assertEquals(synced, baseStore.load().files.getValue("A.kml").cloudId)
    }

    @Test
    fun runPass_persistsBaseAfterEachAction() = runTest(dispatcher) {
        write("A.kml", "a")
        write("B.kml", "b")
        cloud.failingUpload = 2

        assertEquals(PassResult.Failed(SyncError.Transient), engine.runPass())

        assertEquals(setOf("A.kml"), baseStore.load().files.keys)
    }

    @Test
    fun runPass_whileRunning_reportsLastSync() = runTest(dispatcher) {
        engine.runPass()
        val lastSync = now
        now += 60_000
        write("A.kml", "a")
        val upload = CompletableDeferred<Unit>()
        cloud.uploadGate = upload
        val pass = launch { engine.runPass() }

        assertEquals(SyncStatus.Running(lastSync), engine.status.value)
        upload.complete(Unit)
        pass.join()
    }

    @Test
    fun showError_mapsToStatus() = runTest(dispatcher) {
        engine.runPass()

        engine.showError(SyncError.AuthRequired)
        assertEquals(SyncStatus.NeedsSignIn, engine.status.value)
        engine.showError(SyncError.Other("No account"))
        assertEquals(SyncStatus.Error(SyncError.Other("No account")), engine.status.value)
        engine.showError(SyncError.Transient)
        assertEquals(SyncStatus.Idle(now), engine.status.value)
    }

    @Test
    fun onAuthorized_clearsAuthorizationProblem() = runTest(dispatcher) {
        engine.runPass()
        engine.showError(SyncError.AuthRequired)

        engine.onAuthorized()

        assertEquals(SyncStatus.Idle(now), engine.status.value)
    }

    @Test
    fun isRunning_whilePassRuns() = runTest(dispatcher) {
        write("A.kml", "a")
        val upload = CompletableDeferred<Unit>()
        cloud.uploadGate = upload
        val pass = launch { engine.runPass() }

        assertTrue(engine.isRunning)
        upload.complete(Unit)
        pass.join()
        assertFalse(engine.isRunning)
    }

    @Test
    fun runPass_conflictCopyAvoidsCloudNames() = runTest(dispatcher) {
        write("A.kml", "<kml><Placemark>v1</Placemark></kml>")
        engine.runPass()
        write("A.kml", "<kml><Placemark>local v2</Placemark></kml>")
        cloud.files.values.single().content = "<kml><Placemark>cloud v2</Placemark></kml>".toByteArray()
        cloud.put("A (Pixel 6).kml", "<kml><Placemark>from another device</Placemark></kml>")

        engine.runPass()

        assertEquals("<kml><Placemark>local v2</Placemark></kml>", read("A (Pixel 6)1.kml"))
        assertEquals("<kml><Placemark>from another device</Placemark></kml>", read("A (Pixel 6).kml"))
    }

    @Test
    fun runPass_transientError_keepsLastSyncStatus() = runTest(dispatcher) {
        engine.runPass()
        val lastSync = now
        now += 60_000
        cloud.listError = SyncError.Transient

        assertEquals(PassResult.Failed(SyncError.Transient), engine.runPass())

        assertEquals(SyncStatus.Idle(lastSync), engine.status.value)
    }

    @Test
    fun runPass_skipsCloudOverwriteWhenRemoteChangedMeanwhile() = runTest(dispatcher) {
        write("A.kml", "v1")
        engine.runPass()
        write("A.kml", "local v2")
        cloud.beforeCurrentMd5 = { id -> cloud.files.getValue(id).content = "remote v2".toByteArray() }

        engine.runPass()

        assertEquals("remote v2", cloud.content("A.kml"))
        assertEquals(FakeCloudStore.md5("v1".toByteArray()), baseStore.load().files.getValue("A.kml").md5)
    }

    @Test
    fun runPass_accountChange_deletesNothingLocally() = runTest(dispatcher) {
        write("A.kml", "a")
        engine.runPass()
        cloud.files.clear()
        cloud.generation = "gen-of-account-2"
        cloud.put("C.kml", "c")

        engine.runPass()

        assertEquals(listOf("A.kml", "C.kml"), localNames())
        assertEquals(listOf("A.kml", "C.kml"), cloud.names())
        assertTrue(core.trashed.isEmpty())
        assertEquals("gen-of-account-2", baseStore.load().generation)
    }

    @Test
    fun runPass_withoutGeneration_startsOne() = runTest(dispatcher) {
        engine.runPass()

        assertNotNull(cloud.generation)
        assertEquals(cloud.generation, baseStore.load().generation)
    }

    @Test
    fun runPass_cloudDeletedAndRefilledElsewhere_deletesNothingLocally() = runTest(dispatcher) {
        write("A.kml", "a")
        engine.runPass()
        // Another device deleted all cloud data, then a new device uploaded its own list.
        cloud.deleteAll()
        cloud.put("B.kml", "b")

        engine.runPass()

        assertEquals(listOf("A.kml", "B.kml"), localNames())
        assertEquals(listOf("A.kml", "B.kml"), cloud.names())
        assertTrue(core.trashed.isEmpty())
    }

    @Test
    fun runPass_allListsDeletedElsewhere_trashesThemHere() = runTest(dispatcher) {
        write("A.kml", "a")
        engine.runPass()
        cloud.files.values.single().trashedAt = now

        engine.runPass()

        assertEquals(emptyList<String>(), localNames())
        assertEquals(listOf(File(dir, "A.kml")), core.trashed)
        assertTrue(cloud.names().isEmpty())
    }

    @Test
    fun forgetLocalState_waitsForRunningPass() = runTest(dispatcher) {
        write("A.kml", "a")
        val upload = CompletableDeferred<Unit>()
        cloud.uploadGate = upload
        val pass = launch { engine.runPass() }

        val forget = launch { engine.forgetLocalState() }
        upload.complete(Unit)
        joinAll(pass, forget)

        assertEquals(BaseState.EMPTY, baseStore.load())
    }

    @Test
    fun runPass_postponesLocalApplyWhenCoreDefersSaving() = runTest(dispatcher) {
        write("U.kml", "u")
        cloud.put("D.kml", "d")
        core.canApply = false

        assertEquals(PassResult.Deferred, engine.runPass())

        assertEquals(listOf("U.kml"), localNames())
        assertEquals(listOf("D.kml", "U.kml"), cloud.names())
        assertEquals(setOf("U.kml"), baseStore.load().files.keys)
    }

    @Test
    fun runPass_deferred_keepsLastSync() = runTest(dispatcher) {
        engine.runPass()
        val lastSync = now
        now += 60_000
        cloud.put("D.kml", "d")
        core.canApply = false

        assertEquals(PassResult.Deferred, engine.runPass())

        assertEquals(lastSync, baseStore.load().lastSync)
        assertEquals(SyncStatus.Idle(lastSync), engine.status.value)
    }

    @Test
    fun runPass_removesStaleTempFilesAndCompletesInterruptedDownload() = runTest(dispatcher) {
        tmpDir.mkdirs()
        File(tmpDir, "download123.part").writeText("partial")
        cloud.put("D.kml", "d")

        engine.runPass()

        assertEquals("d", read("D.kml"))
        assertEquals(0, tmpDir.list()!!.size)
    }

    @Test
    fun runPass_trashesDuplicateWithoutPlacemarksWithoutLocalCopy() = runTest(dispatcher) {
        cloud.put("My Places.kml", "<kml><Placemark/></kml>", "2026-10-01T10:00:00.000Z")
        cloud.put("My Places.kml", "<kml><Document><name>My Places</name></Document></kml>", "2026-10-02T10:00:00.000Z")

        engine.runPass()

        assertEquals(listOf("My Places.kml"), localNames())
        assertEquals("<kml><Placemark/></kml>", read("My Places.kml"))
        assertEquals(listOf("My Places.kml"), cloud.names())
        assertEquals(1, cloud.trashed().size)
    }

    @Test
    fun runPass_keepsDuplicateWithPlacemarksAsLocalCopy() = runTest(dispatcher) {
        cloud.put("Paris.kml", "<kml><Placemark>a</Placemark></kml>", "2026-10-01T10:00:00.000Z")
        cloud.put("Paris.kml", "<kml><Placemark>b</Placemark></kml>", "2026-10-02T10:00:00.000Z")

        engine.runPass()

        assertEquals(listOf("Paris.kml", "Paris1.kml"), localNames())
        assertEquals("<kml><Placemark>b</Placemark></kml>", read("Paris1.kml"))
        assertEquals(1, cloud.trashed().size)
    }

    @Test
    fun runPass_trashesCloudFileDeletedLocally() = runTest(dispatcher) {
        write("A.kml", "a")
        engine.runPass()
        File(dir, "A.kml").delete()

        engine.runPass()

        assertTrue(cloud.names().isEmpty())
        assertEquals(FakeCloudStore.TRASH_FOLDER_ID, baseStore.load().trashFolderId)
        assertFalse("A.kml" in baseStore.load().files)
    }

    @Test
    fun runPass_purgesTrashOlderThan30Days() = runTest(dispatcher) {
        cloud.generation = "gen"
        baseStore.save(BaseState("gen", FakeCloudStore.TRASH_FOLDER_ID, null, emptyMap()))
        write("Keep.kml", "k")
        cloud.put("Keep.kml", "k")
        val old = cloud.put("Old.kml", "o")
        val recent = cloud.put("Recent.kml", "r")
        cloud.files.getValue(old).trashedAt = now - 31L * DAY
        cloud.files.getValue(recent).trashedAt = now - DAY

        engine.runPass()

        assertFalse(old in cloud.files)
        assertTrue(recent in cloud.files)
    }

    @Test
    fun runPass_quotaExceeded_setsErrorStatus() = runTest(dispatcher) {
        cloud.listError = SyncError.QuotaExceeded

        assertEquals(PassResult.Failed(SyncError.QuotaExceeded), engine.runPass())
        assertEquals(SyncStatus.Error(SyncError.QuotaExceeded), engine.status.value)
    }

    @Test
    fun runPass_authRequired_setsNeedsSignIn() = runTest(dispatcher) {
        cloud.listError = SyncError.AuthRequired

        assertEquals(PassResult.Failed(SyncError.AuthRequired), engine.runPass())
        assertEquals(SyncStatus.NeedsSignIn, engine.status.value)
    }

    @Test
    fun deleteCloudData_clearsCloudAndBase() = runTest(dispatcher) {
        write("A.kml", "a")
        engine.runPass()

        engine.deleteCloudData()

        assertTrue(cloud.names().isEmpty())
        assertNull(baseStore.load().generation)
        assertEquals(listOf("A.kml"), localNames())
    }

    /** This device synced "My Places" as a newer duplicate of a list another device created first. */
    private fun loseNameRace() {
        cloud.generation = "gen"
        cloud.put("My Places.kml", "<kml><Placemark>a</Placemark></kml>", "2026-10-01T10:00:00.000Z")
        val own = cloud.put("My Places.kml", "<kml><Placemark>b</Placemark></kml>", "2026-10-02T10:00:00.000Z")
        write("My Places.kml", "<kml><Placemark>b</Placemark></kml>")
        core.names["My Places.kml"] = "My Places"
        val md5 = FakeCloudStore.md5("<kml><Placemark>b</Placemark></kml>".toByteArray())
        baseStore.save(BaseState("gen", null, null, mapOf("My Places.kml" to BaseEntry(own, md5))))
    }

    @Test
    fun runPass_lostNameRace_keepsLocalListAsCopy() = runTest(dispatcher) {
        loseNameRace()

        engine.runPass()

        assertEquals("<kml><Placemark>a</Placemark></kml>", read("My Places.kml"))
        assertEquals("<kml><Placemark>b</Placemark></kml>", read("My Places (Pixel 6).kml"))
        assertTrue(core.trashed.isEmpty())
        assertEquals(1, cloud.trashed().size)
    }

    @Test
    fun runPass_lostNameRaceWhileCopyIsPostponed_keepsOwnCloudFile() = runTest(dispatcher) {
        loseNameRace()
        core.canApply = false

        assertEquals(PassResult.Deferred, engine.runPass())
        assertTrue(cloud.trashed().isEmpty())

        core.canApply = true
        engine.runPass()

        assertEquals("<kml><Placemark>a</Placemark></kml>", read("My Places.kml"))
        assertEquals("<kml><Placemark>b</Placemark></kml>", read("My Places (Pixel 6).kml"))
        assertEquals(1, cloud.trashed().size)
    }

    @Test
    fun runPass_coreStartsDeferringDuringDownload_reportsDeferred() = runTest(dispatcher) {
        write("A.kml", "a")
        engine.runPass()
        cloud.files.values.single().content = "a2".toByteArray()
        cloud.beforeDownload = { core.canApply = false }

        assertEquals(PassResult.Deferred, engine.runPass())

        assertEquals("a", read("A.kml"))
    }

    @Test
    fun runPass_conflictWithEmptyCloudList_uploadsLocalInsteadOfCopy() = runTest(dispatcher) {
        cloud.put("My Places.kml", "<kml><Document><name>My Places</name></Document></kml>")
        write("My Places.kml", "<kml><Placemark>mine</Placemark></kml>")

        engine.runPass()

        assertEquals(listOf("My Places.kml"), localNames())
        assertEquals("<kml><Placemark>mine</Placemark></kml>", cloud.content("My Places.kml"))
        assertEquals("<kml><Placemark>mine</Placemark></kml>", read("My Places.kml"))
        assertEquals(setOf("My Places.kml"), baseStore.load().files.keys)
    }

    @Test
    fun runPass_cloudBeingDeletedElsewhere_deletesNothingLocally() = runTest(dispatcher) {
        write("A.kml", "a")
        write("B.kml", "b")
        engine.runPass()
        // Another device deletes all data: the generation first, then the files one by one.
        val deleted = cloud.files.keys.toList()
        cloud.generation = null
        cloud.files.remove(deleted.first())

        engine.runPass()
        cloud.files.keys.removeAll(deleted)
        engine.runPass()

        assertEquals(listOf("A.kml", "B.kml"), localNames())
        assertEquals(listOf("A.kml", "B.kml"), cloud.names())
        assertTrue(core.trashed.isEmpty())
    }

    @Test
    fun deleteCloudData_forgetsBaseBeforeDeletingCloud() = runTest(dispatcher) {
        write("A.kml", "a")
        engine.runPass()
        cloud.deleteAllError = SyncError.Transient

        try {
            engine.deleteCloudData()
        } catch (e: CloudException) {
            assertEquals(SyncError.Transient, e.error)
        }

        assertEquals(BaseState.EMPTY, baseStore.load())
    }

    @Test
    fun runPass_unexpectedException_setsErrorStatus() = runTest(dispatcher) {
        write("A.kml", "a")
        File(dir, "A.kml").setReadable(false)
        try {
            val result = engine.runPass()

            assertTrue(result is PassResult.Failed)
            assertTrue(engine.status.value is SyncStatus.Error)
        } finally {
            File(dir, "A.kml").setReadable(true)
        }
    }

    @Test
    fun runPass_fileMissingFromListing_keepsLocalList() = runTest(dispatcher) {
        write("A.kml", "a")
        engine.runPass()
        cloud.hiddenFromListing += "A.kml"

        engine.runPass()

        assertEquals(listOf("A.kml"), localNames())
        assertTrue(core.trashed.isEmpty())
        assertTrue("A.kml" in baseStore.load().files)
    }

    @Test
    fun runPass_fileMissingFromListing_uploadsEditToSameFile() = runTest(dispatcher) {
        write("A.kml", "a")
        engine.runPass()
        val id = cloud.files.keys.single()
        cloud.hiddenFromListing += "A.kml"
        write("A.kml", "a2")

        engine.runPass()

        assertEquals(listOf(id), cloud.files.keys.toList())
        assertEquals("a2", cloud.content("A.kml"))
    }

    @Test
    fun runPass_editedLocallyDeletedRemotely_uploadsAgain() = runTest(dispatcher) {
        write("A.kml", "a")
        engine.runPass()
        cloud.files.values.single().trashedAt = now
        write("A.kml", "a2")

        engine.runPass()

        assertEquals("a2", cloud.content("A.kml"))
        assertEquals(listOf("A.kml"), localNames())
    }

    @Test
    fun runPass_fileDeletedBeforeDownload_skipsIt() = runTest(dispatcher) {
        cloud.put("D.kml", "d")
        cloud.beforeDownload = { id -> cloud.files.remove(id) }

        assertEquals(PassResult.Synced, engine.runPass())

        assertTrue(localNames().isEmpty())
        assertTrue(baseStore.load().files.isEmpty())
    }

    @Test
    fun runPass_duplicateCopyAvoidsNamesOfCloudFiles() = runTest(dispatcher) {
        cloud.put("Paris.kml", "<kml><Placemark>a</Placemark></kml>", "2026-10-01T10:00:00.000Z")
        cloud.put("Paris.kml", "<kml><Placemark>b</Placemark></kml>", "2026-10-02T10:00:00.000Z")
        cloud.put("Paris1.kml", "<kml><Placemark>c</Placemark></kml>")

        engine.runPass()

        assertEquals(listOf("Paris.kml", "Paris1.kml", "Paris2.kml"), localNames())
        assertEquals("<kml><Placemark>c</Placemark></kml>", read("Paris1.kml"))
        assertEquals("<kml><Placemark>b</Placemark></kml>", read("Paris2.kml"))
    }

    private companion object {
        const val DAY = 24L * 60 * 60 * 1000
    }
}
