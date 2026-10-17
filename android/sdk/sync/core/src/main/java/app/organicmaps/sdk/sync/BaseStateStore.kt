package app.organicmaps.sdk.sync

import java.io.File
import org.json.JSONException
import org.json.JSONObject

/** Persists [BaseState] as JSON. A missing or unreadable file means a first synchronization, which is safe. */
class BaseStateStore(private val file: File) {
    fun load(): BaseState {
        if (!file.isFile) return BaseState.EMPTY
        return try {
            parse(JSONObject(file.readText()))
        } catch (ignored: JSONException) {
            BaseState.EMPTY
        }
    }

    fun save(state: BaseState) {
        file.parentFile?.mkdirs()
        val tmp = File(file.path + ".tmp")
        tmp.writeText(serialize(state).toString())
        check(tmp.renameTo(file)) { "Can't rename $tmp to $file" }
    }

    fun delete() {
        file.delete()
    }

    private fun parse(json: JSONObject): BaseState {
        if (json.optInt(KEY_VERSION) != VERSION) return BaseState.EMPTY
        val files = json.getJSONObject(KEY_FILES)
        return BaseState(
            account = json.optStringOrNull(KEY_ACCOUNT),
            trashFolderId = json.optStringOrNull(KEY_TRASH_FOLDER_ID),
            lastSync = if (json.isNull(KEY_LAST_SYNC)) null else json.getLong(KEY_LAST_SYNC),
            files = files.keys().asSequence().associateWith { name ->
                val entry = files.getJSONObject(name)
                BaseEntry(entry.getString(KEY_CLOUD_ID), entry.getString(KEY_MD5))
            },
        )
    }

    private fun serialize(state: BaseState) = JSONObject().apply {
        put(KEY_VERSION, VERSION)
        put(KEY_ACCOUNT, state.account ?: JSONObject.NULL)
        put(KEY_TRASH_FOLDER_ID, state.trashFolderId ?: JSONObject.NULL)
        put(KEY_LAST_SYNC, state.lastSync ?: JSONObject.NULL)
        put(
            KEY_FILES,
            JSONObject().apply {
                for ((name, entry) in state.files) {
                    put(name, JSONObject().put(KEY_CLOUD_ID, entry.cloudId).put(KEY_MD5, entry.md5))
                }
            },
        )
    }

    private fun JSONObject.optStringOrNull(key: String) = if (isNull(key)) null else getString(key)

    private companion object {
        const val VERSION = 1
        const val KEY_VERSION = "version"
        const val KEY_ACCOUNT = "account"
        const val KEY_TRASH_FOLDER_ID = "trashFolderId"
        const val KEY_LAST_SYNC = "lastSync"
        const val KEY_FILES = "files"
        const val KEY_CLOUD_ID = "cloudId"
        const val KEY_MD5 = "md5"
    }
}
