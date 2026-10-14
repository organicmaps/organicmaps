package app.organicmaps.sdk.sync

import app.organicmaps.sdk.sync.SyncAction.ConflictCopy
import app.organicmaps.sdk.sync.SyncAction.Download
import app.organicmaps.sdk.sync.SyncAction.DropBase
import app.organicmaps.sdk.sync.SyncAction.RecordBase
import app.organicmaps.sdk.sync.SyncAction.ResolveDuplicate
import app.organicmaps.sdk.sync.SyncAction.TrashCloud
import app.organicmaps.sdk.sync.SyncAction.TrashLocal
import app.organicmaps.sdk.sync.SyncAction.Upload

/** Decides what to do with every file by comparing the local and cloud MD5 with the last synchronized one. */
object Reconciler {
    fun plan(local: List<LocalFile>, cloud: List<CloudFile>, baseFiles: Map<String, BaseEntry>): List<SyncAction> {
        val actions = mutableListOf<SyncAction>()
        val localByName = local.associateBy { it.name }
        val localMd5s = local.mapTo(HashSet()) { it.md5 }
        val cloudByName = HashMap<String, CloudFile>()
        for ((name, files) in cloud.groupBy { it.name }) {
            val owner = files.minWith(compareBy<CloudFile>({ it.createdTime }, { it.id }))
            cloudByName[name] = owner
            for (duplicate in files.filter { it !== owner }) {
                val isKnown = duplicate.md5 == owner.md5 || duplicate.md5 in localMd5s
                actions += ResolveDuplicate(name, duplicate, keepAsLocalCopy = !isKnown)
            }
        }

        val cloudIds = cloud.mapTo(HashSet()) { it.id }
        for (name in localByName.keys + cloudByName.keys + baseFiles.keys) {
            val localFile = localByName[name]
            val cloudFile = cloudByName[name]
            val baseEntry = baseFiles[name]
            // A base entry describes one cloud file, so it applies only while no other file owns the name.
            val action = if (baseEntry == null || cloudFile == null || baseEntry.cloudId == cloudFile.id) {
                decide(name, localFile, cloudFile, baseEntry)
            } else {
                decideReplaced(name, localFile, cloudFile, baseEntry, baseEntry.cloudId in cloudIds)
            }
            action?.let { actions += it }
        }
        return actions.sortedWith(compareBy({ kindOrder(it) }, { it.name }))
    }

    /**
     * Another cloud file owns the name. While the synced one is still listed, this device's upload lost a duplicate
     * race: compare as if never synced, so the local version is kept as a copy. Otherwise the synced file was deleted
     * elsewhere and a new one took its name, which an unchanged local file follows like any remote deletion.
     */
    private fun decideReplaced(
        name: String,
        local: LocalFile?,
        cloud: CloudFile,
        base: BaseEntry,
        isBaseListed: Boolean,
    ): SyncAction? = if (!isBaseListed && local?.md5 == base.md5 && base.md5 != cloud.md5) {
        Download(name, cloud, base.md5, replacedCloudId = base.cloudId)
    } else {
        decideWithoutBase(name, local, cloud)
    }

    private fun decide(name: String, local: LocalFile?, cloud: CloudFile?, base: BaseEntry?): SyncAction? {
        if (base == null) return decideWithoutBase(name, local, cloud)
        if (local != null && cloud != null) return decideBothPresent(name, local, cloud, base)
        if (local != null) {
            return if (local.md5 == base.md5) {
                TrashLocal(name, local.md5, base.cloudId)
            } else {
                Upload(name, base.cloudId)
            }
        }
        if (cloud != null) {
            return if (cloud.md5 == base.md5) TrashCloud(name, cloud.id, cloud.md5) else Download(name, cloud, null)
        }
        return DropBase(name)
    }

    private fun decideWithoutBase(name: String, local: LocalFile?, cloud: CloudFile?): SyncAction? = when {
        local != null && cloud != null -> if (local.md5 == cloud.md5) {
            RecordBase(name, BaseEntry(cloud.id, cloud.md5))
        } else {
            ConflictCopy(name, cloud, local.md5, hasBase = false)
        }

        local != null -> Upload(name, null)

        cloud != null -> Download(name, cloud, null)

        else -> null
    }

    private fun decideBothPresent(name: String, local: LocalFile, cloud: CloudFile, base: BaseEntry): SyncAction? {
        val localChanged = local.md5 != base.md5
        val cloudChanged = cloud.md5 != base.md5
        return when {
            !localChanged && !cloudChanged -> null
            !cloudChanged -> Upload(name, cloud.id)
            !localChanged -> Download(name, cloud, local.md5)
            local.md5 == cloud.md5 -> RecordBase(name, BaseEntry(cloud.id, cloud.md5))
            else -> ConflictCopy(name, cloud, local.md5, hasBase = true)
        }
    }

    // Copies first so that nothing overwrites a local version before it is saved; deletions last.
    private fun kindOrder(action: SyncAction): Int = when (action) {
        is ConflictCopy -> 0
        is ResolveDuplicate -> 1
        is Download -> 2
        is Upload -> 3
        is TrashCloud, is TrashLocal -> 4
        is RecordBase, is DropBase -> 5
    }
}
