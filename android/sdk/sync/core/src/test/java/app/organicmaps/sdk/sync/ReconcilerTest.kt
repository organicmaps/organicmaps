package app.organicmaps.sdk.sync

import app.organicmaps.sdk.sync.SyncAction.ConflictCopy
import app.organicmaps.sdk.sync.SyncAction.Download
import app.organicmaps.sdk.sync.SyncAction.DropBase
import app.organicmaps.sdk.sync.SyncAction.RecordBase
import app.organicmaps.sdk.sync.SyncAction.ResolveDuplicate
import app.organicmaps.sdk.sync.SyncAction.TrashCloud
import app.organicmaps.sdk.sync.SyncAction.TrashLocal
import app.organicmaps.sdk.sync.SyncAction.Upload
import org.junit.Assert.assertEquals
import org.junit.Test

class ReconcilerTest {
    private fun local(name: String, md5: String) = LocalFile(name, md5)

    private fun cloud(name: String, id: String, md5: String, created: String = "2026-10-01T10:00:00.000Z") =
        CloudFile(id, name, md5, created)

    private fun base(vararg entries: Pair<String, BaseEntry>) = mapOf(*entries)

    private fun plan(local: List<LocalFile>, cloud: List<CloudFile>, base: Map<String, BaseEntry> = emptyMap()) =
        Reconciler.plan(local, cloud, base)

    @Test
    fun withBase_unchanged_nothing() {
        val plan =
            plan(
                listOf(local("A.kml", "b1")),
                listOf(cloud("A.kml", "id1", "b1")),
                base("A.kml" to BaseEntry("id1", "b1")),
            )
        assertEquals(emptyList<SyncAction>(), plan)
    }

    @Test
    fun withBase_localChanged_cloudSame_uploadsToSameCloudId() {
        val plan =
            plan(
                listOf(local("A.kml", "l2")),
                listOf(cloud("A.kml", "id1", "b1")),
                base("A.kml" to BaseEntry("id1", "b1")),
            )
        assertEquals(listOf(Upload("A.kml", "id1")), plan)
    }

    @Test
    fun withBase_cloudChanged_localSame_downloads() {
        val c = cloud("A.kml", "id1", "c2")
        val plan = plan(listOf(local("A.kml", "b1")), listOf(c), base("A.kml" to BaseEntry("id1", "b1")))
        assertEquals(listOf(Download("A.kml", c, "b1")), plan)
    }

    @Test
    fun withBase_bothChangedEqually_recordsBase() {
        val plan =
            plan(
                listOf(local("A.kml", "x")),
                listOf(cloud("A.kml", "id1", "x")),
                base("A.kml" to BaseEntry("id1", "b1")),
            )
        assertEquals(listOf(RecordBase("A.kml", BaseEntry("id1", "x"))), plan)
    }

    @Test
    fun withBase_bothChangedDifferently_conflictCopy() {
        val c = cloud("A.kml", "id1", "c2")
        val plan = plan(listOf(local("A.kml", "l2")), listOf(c), base("A.kml" to BaseEntry("id1", "b1")))
        assertEquals(listOf(ConflictCopy("A.kml", c, "l2", hasBase = true)), plan)
    }

    @Test
    fun withBase_deletedLocally_trashesCloud() {
        val plan = plan(
            emptyList(),
            listOf(cloud("A.kml", "id1", "b1"), cloud("B.kml", "id2", "b2")),
            base("A.kml" to BaseEntry("id1", "b1"), "B.kml" to BaseEntry("id2", "b2")),
        )
        assertEquals(listOf(TrashCloud("A.kml", "id1", "b1"), TrashCloud("B.kml", "id2", "b2")), plan)
    }

    @Test
    fun withBase_deletedLocallyEditedRemotely_downloads() {
        val c = cloud("A.kml", "id1", "c2")
        val plan = plan(emptyList(), listOf(c), base("A.kml" to BaseEntry("id1", "b1")))
        assertEquals(listOf(Download("A.kml", c, null)), plan)
    }

    @Test
    fun withBase_deletedRemotely_trashesLocal() {
        val plan = plan(
            listOf(local("A.kml", "b1"), local("B.kml", "b2")),
            listOf(cloud("B.kml", "id2", "b2")),
            base("A.kml" to BaseEntry("id1", "b1"), "B.kml" to BaseEntry("id2", "b2")),
        )
        assertEquals(listOf(TrashLocal("A.kml", "b1", "id1")), plan)
    }

    @Test
    fun withBase_deletedRemotelyEditedLocally_uploadsAgain() {
        val plan = plan(
            listOf(local("A.kml", "l2"), local("B.kml", "b2")),
            listOf(cloud("B.kml", "id2", "b2")),
            base("A.kml" to BaseEntry("id1", "b1"), "B.kml" to BaseEntry("id2", "b2")),
        )
        assertEquals(listOf(Upload("A.kml", "id1")), plan)
    }

    @Test
    fun withBase_goneEverywhere_dropsBase() {
        val plan = plan(
            listOf(local("B.kml", "b2")),
            listOf(cloud("B.kml", "id2", "b2")),
            base("A.kml" to BaseEntry("id1", "b1"), "B.kml" to BaseEntry("id2", "b2")),
        )
        assertEquals(listOf(DropBase("A.kml")), plan)
    }

    @Test
    fun noBase_localOnly_uploads() {
        val plan = plan(listOf(local("A.kml", "l1")), emptyList())
        assertEquals(listOf(Upload("A.kml", null)), plan)
    }

    @Test
    fun noBase_cloudOnly_downloads() {
        val c = cloud("A.kml", "id1", "c1")
        val plan = plan(emptyList(), listOf(c))
        assertEquals(listOf(Download("A.kml", c, null)), plan)
    }

    @Test
    fun noBase_equal_recordsBase() {
        val plan = plan(listOf(local("A.kml", "x")), listOf(cloud("A.kml", "id1", "x")))
        assertEquals(listOf(RecordBase("A.kml", BaseEntry("id1", "x"))), plan)
    }

    @Test
    fun noBase_different_conflictCopy() {
        val c = cloud("A.kml", "id1", "c1")
        val plan = plan(listOf(local("A.kml", "l1")), listOf(c))
        assertEquals(listOf(ConflictCopy("A.kml", c, "l1", hasBase = false)), plan)
    }

    @Test
    fun duplicates_oldestCreatedTimeOwnsName() {
        val newer = cloud("A.kml", "id1", "n1", "2026-10-02T10:00:00.000Z")
        val older = cloud("A.kml", "id9", "o1", "2026-10-01T10:00:00.000Z")
        val plan = plan(emptyList(), listOf(newer, older))
        assertEquals(listOf(Download("A.kml", older, null), ResolveDuplicate("A.kml", newer, true)), plan)
    }

    @Test
    fun duplicates_sameCreatedTime_smallestIdOwnsName() {
        val a = cloud("A.kml", "idB", "x")
        val b = cloud("A.kml", "idA", "y")
        val plan = plan(emptyList(), listOf(a, b))
        assertEquals(listOf(Download("A.kml", b, null), ResolveDuplicate("A.kml", a, true)), plan)
    }

    @Test
    fun duplicates_sameContentAsOwner_trashedWithoutCopy() {
        val owner = cloud("A.kml", "id1", "x", "2026-10-01T10:00:00.000Z")
        val dup = cloud("A.kml", "id2", "x", "2026-10-02T10:00:00.000Z")
        val plan = plan(listOf(local("A.kml", "x")), listOf(owner, dup))
        assertEquals(
            listOf(RecordBase("A.kml", BaseEntry("id1", "x")), ResolveDuplicate("A.kml", dup, false)),
            plan,
        )
    }

    @Test
    fun duplicates_sameContentAsAnyLocalFile_trashedWithoutCopy() {
        val owner = cloud("A.kml", "id1", "x", "2026-10-01T10:00:00.000Z")
        val dup = cloud("A.kml", "id2", "y", "2026-10-02T10:00:00.000Z")
        val plan = plan(listOf(local("A.kml", "x"), local("A1.kml", "y")), listOf(owner, dup))
        assertEquals(ResolveDuplicate("A.kml", dup, false), plan.last())
    }

    @Test
    fun duplicates_differentContent_keptAsLocalCopy() {
        val owner = cloud("A.kml", "id1", "x", "2026-10-01T10:00:00.000Z")
        val dup = cloud("A.kml", "id2", "z", "2026-10-02T10:00:00.000Z")
        val plan = plan(listOf(local("A.kml", "x")), listOf(owner, dup))
        assertEquals(ResolveDuplicate("A.kml", dup, true), plan.last())
    }

    @Test
    fun actions_areOrderedByKind() {
        val conflict = cloud("C.kml", "idC", "c2")
        val download = cloud("D.kml", "idD", "d1")
        val trash = cloud("T.kml", "idT", "t1")
        val plan = plan(
            listOf(local("C.kml", "l2"), local("U.kml", "u1"), local("L.kml", "lb")),
            listOf(conflict, download, trash, cloud("R.kml", "idR", "r")),
            base(
                "C.kml" to BaseEntry("idC", "cb"),
                "T.kml" to BaseEntry("idT", "t1"),
                "L.kml" to BaseEntry("idL", "lb"),
                "G.kml" to BaseEntry("idG", "g"),
            ),
        )
        assertEquals(
            listOf(
                ConflictCopy("C.kml", conflict, "l2", hasBase = true),
                Download("D.kml", download, null),
                Download("R.kml", cloud("R.kml", "idR", "r"), null),
                Upload("U.kml", null),
                TrashLocal("L.kml", "lb", "idL"),
                TrashCloud("T.kml", "idT", "t1"),
                DropBase("G.kml"),
            ),
            plan,
        )
    }

    @Test
    fun duplicates_ownUploadLostName_keepsLocalAsCopy() {
        val owner = cloud("My Places.kml", "idA", "mdA", "2026-10-01T10:00:00.000Z")
        val own = cloud("My Places.kml", "idB", "mdB", "2026-10-02T10:00:00.000Z")
        val plan =
            plan(
                listOf(local("My Places.kml", "mdB")),
                listOf(owner, own),
                base("My Places.kml" to BaseEntry("idB", "mdB")),
            )
        assertEquals(
            listOf(
                ConflictCopy("My Places.kml", owner, "mdB", hasBase = false),
                ResolveDuplicate("My Places.kml", own, false),
            ),
            plan,
        )
    }

    @Test
    fun duplicates_ownUploadEditedSinceThen_trashedWithoutCopy() {
        val owner = cloud("A.kml", "idA", "mdA", "2026-10-01T10:00:00.000Z")
        val own = cloud("A.kml", "idB", "mdB", "2026-10-02T10:00:00.000Z")
        val plan = plan(listOf(local("A.kml", "mdB2")), listOf(owner, own), base("A.kml" to BaseEntry("idB", "mdB")))
        assertEquals(
            listOf(ConflictCopy("A.kml", owner, "mdB2", hasBase = false), ResolveDuplicate("A.kml", own, false)),
            plan,
        )
    }

    @Test
    fun withBase_syncedFileReplacedElsewhere_unchangedHere_downloadsNewOne() {
        val other = cloud("A.kml", "id2", "c2")
        val plan = plan(listOf(local("A.kml", "b1")), listOf(other), base("A.kml" to BaseEntry("id1", "b1")))
        assertEquals(listOf(Download("A.kml", other, "b1", replacedCloudId = "id1")), plan)
    }

    @Test
    fun withBase_syncedFileReplacedElsewhere_editedHere_keepsLocalAsCopy() {
        val other = cloud("A.kml", "id2", "c2")
        val plan = plan(listOf(local("A.kml", "l2")), listOf(other), base("A.kml" to BaseEntry("id1", "b1")))
        assertEquals(listOf(ConflictCopy("A.kml", other, "l2", hasBase = false)), plan)
    }

    @Test
    fun withBase_syncedFileReplacedElsewhere_sameContent_recordsBase() {
        val other = cloud("A.kml", "id2", "b1")
        val plan = plan(listOf(local("A.kml", "b1")), listOf(other), base("A.kml" to BaseEntry("id1", "b1")))
        assertEquals(listOf(RecordBase("A.kml", BaseEntry("id2", "b1"))), plan)
    }
}
