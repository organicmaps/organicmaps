package app.organicmaps.sdk.sync

import app.organicmaps.sdk.sync.testing.FakeCoreBookmarks
import java.io.File
import java.io.IOException
import java.nio.file.Files
import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import kotlinx.coroutines.test.UnconfinedTestDispatcher
import kotlinx.coroutines.test.runTest
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test

@OptIn(ExperimentalCoroutinesApi::class)
class LocalStoreTest {
    private lateinit var dir: File
    private lateinit var tmpDir: File
    private val core = FakeCoreBookmarks()
    private val dispatcher = UnconfinedTestDispatcher()
    private lateinit var store: LocalStore

    @Before
    fun setUp() {
        val root = Files.createTempDirectory("sync").toFile()
        dir = File(root, "bookmarks").apply { mkdirs() }
        tmpDir = File(root, "sync/tmp")
        store = LocalStore(dir, tmpDir, core, dispatcher, dispatcher)
    }

    private fun write(name: String, text: String) = File(dir, name).apply { writeText(text) }

    private fun temp(text: String) = store.newTempFile().apply { writeText(text) }

    @Test
    fun snapshot_ignoresHiddenTmpAndNonKml() = runTest(dispatcher) {
        val kml = write("A.kml", "a")
        write(".x.kml", "x")
        write("A.kml.tmp123", "t")
        write("B.gpx", "b")
        File(dir, "D.kml").mkdirs()
        core.emptyCategories += "A.kml"

        assertEquals(listOf(LocalFile("A.kml", Md5.of(kml), isEmptyCategory = true)), store.snapshot())
    }

    @Test
    fun install_replacesAndReloads() = runTest(dispatcher) {
        val target = write("A.kml", "old")
        val tmp = temp("new")

        assertTrue(store.install("A.kml", tmp, Md5.of(target)))

        assertEquals("new", target.readText())
        assertEquals(listOf(target), core.reloaded)
    }

    @Test
    fun install_skipsWhenTargetChanged() = runTest(dispatcher) {
        val expected = Md5.of(write("A.kml", "old"))
        write("A.kml", "edited meanwhile")
        val tmp = temp("new")

        assertFalse(store.install("A.kml", tmp, expected))

        assertEquals("edited meanwhile", File(dir, "A.kml").readText())
        assertTrue(core.reloaded.isEmpty())
    }

    @Test
    fun install_withoutExpectedMd5_requiresAbsentTarget() = runTest(dispatcher) {
        write("A.kml", "appeared meanwhile")

        assertFalse(store.install("A.kml", temp("new"), null))
        assertTrue(store.install("B.kml", temp("new"), null))
        assertEquals("new", File(dir, "B.kml").readText())
    }

    @Test
    fun copyForConflict_copiesBytesAndRenamesCategory() = runTest(dispatcher) {
        write("Paris.kml", "local version")
        core.names["Paris.kml"] = "Paris"

        val copyName = store.copyForConflict("Paris.kml", "Pixel 6")

        assertEquals("Paris (Pixel 6).kml", copyName)
        assertEquals("local version", File(dir, copyName).readText())
        assertEquals("Paris (Pixel 6)", core.names[copyName])
        assertEquals(listOf(File(dir, copyName)), core.reloaded)
    }

    @Test
    fun installUnique_takesFreeName() = runTest(dispatcher) {
        write("Paris.kml", "owner")

        assertEquals("Paris1.kml", store.installUnique("Paris.kml", temp("duplicate")))
        assertEquals("duplicate", File(dir, "Paris1.kml").readText())
    }

    @Test
    fun installUnique_neverTakesTheOriginalName() = runTest(dispatcher) {
        assertEquals("Paris1.kml", store.installUnique("Paris.kml", temp("duplicate")))
        assertFalse(File(dir, "Paris.kml").exists())
    }

    @Test
    fun trash_movesUnchangedFile() = runTest(dispatcher) {
        val target = write("A.kml", "a")

        assertTrue(store.trash("A.kml", Md5.of(target)))
        assertEquals(listOf(target), core.trashed)
    }

    @Test
    fun trash_skipsWhenMd5Changed() = runTest(dispatcher) {
        write("A.kml", "edited")

        assertFalse(store.trash("A.kml", "0123"))
        assertTrue(core.trashed.isEmpty())
    }

    @Test
    fun install_skipsWhenCoreDefersSaving() = runTest(dispatcher) {
        val target = write("A.kml", "old")
        core.canApply = false

        assertFalse(store.install("A.kml", temp("new"), Md5.of(target)))
        assertEquals("old", target.readText())
    }

    @Test
    fun trash_skipsWhenCoreDefersSaving() = runTest(dispatcher) {
        val target = write("A.kml", "a")
        core.canApply = false

        assertFalse(store.trash("A.kml", Md5.of(target)))
        assertTrue(target.exists())
    }

    @Test
    fun install_reloadsEvenIfCancelledAfterRename() = runTest(dispatcher) {
        val target = write("A.kml", "old")
        val tmp = temp("new")
        lateinit var job: Job
        core.afterFileThread = { job.cancel() }
        job = launch(start = CoroutineStart.LAZY) { store.install("A.kml", tmp, Md5.of(target)) }

        job.join()

        assertEquals("new", target.readText())
        assertEquals(listOf(target), core.reloaded)
    }

    @Test
    fun snapshot_missingDirectory_isEmpty() = runTest(dispatcher) {
        dir.deleteRecursively()

        assertTrue(store.snapshot().isEmpty())
    }

    @Test(expected = IOException::class)
    fun snapshot_unreadableDirectory_throws() = runTest(dispatcher) {
        write("A.kml", "a")
        dir.setReadable(false)
        try {
            store.snapshot()
        } finally {
            dir.setReadable(true)
        }
    }

    @Test
    fun cleanTemp_removesLeftovers() {
        temp("partial download")

        store.cleanTemp()

        assertEquals(0, tmpDir.listFiles()?.size ?: 0)
    }
}
