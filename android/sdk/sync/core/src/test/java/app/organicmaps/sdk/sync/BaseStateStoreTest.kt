package app.organicmaps.sdk.sync

import java.io.File
import java.nio.file.Files
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test

class BaseStateStoreTest {
    private lateinit var file: File
    private lateinit var store: BaseStateStore

    @Before
    fun setUp() {
        file = File(Files.createTempDirectory("sync").toFile(), "sync/google_drive.json")
        store = BaseStateStore(file)
    }

    @Test
    fun saveThenLoad_roundTripsUnicodeNames() {
        val state = BaseState(
            account = "perm-1",
            trashFolderId = "trash-1",
            lastSync = 1_700_000_000_000,
            files = mapOf("Café ☕.kml" to BaseEntry("id1", "md5a"), "B.kml" to BaseEntry("id2", "md5b")),
        )

        store.save(state)

        assertEquals(state, BaseStateStore(file).load())
    }

    @Test
    fun saveThenLoad_keepsNullFields() {
        store.save(BaseState.EMPTY.copy(account = "perm-1"))

        assertEquals(BaseState.EMPTY.copy(account = "perm-1"), store.load())
    }

    @Test
    fun load_missingFile_returnsEmpty() {
        assertEquals(BaseState.EMPTY, store.load())
    }

    @Test
    fun load_corruptFile_returnsEmpty() {
        file.parentFile!!.mkdirs()
        file.writeText("{not json")

        assertEquals(BaseState.EMPTY, store.load())
    }

    @Test
    fun load_unknownVersion_returnsEmpty() {
        file.parentFile!!.mkdirs()
        file.writeText("""{"version": 2, "files": {}}""")

        assertEquals(BaseState.EMPTY, store.load())
    }

    @Test
    fun save_isAtomic_noTempLeft() {
        store.save(BaseState.EMPTY)

        assertEquals(listOf(file.name), file.parentFile!!.list()!!.toList())
    }

    @Test
    fun delete_removesFile() {
        store.save(BaseState.EMPTY)

        store.delete()

        assertEquals(BaseState.EMPTY, store.load())
        assertEquals(false, file.exists())
    }
}
