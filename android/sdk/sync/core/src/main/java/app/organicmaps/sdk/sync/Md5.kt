package app.organicmaps.sdk.sync

import java.io.File
import java.security.MessageDigest

/** Lowercase hex MD5 of the file content, as Google Drive reports it in md5Checksum. */
object Md5 {
    fun of(file: File): String {
        val digest = MessageDigest.getInstance("MD5")
        file.forEachBlock { buffer, bytesRead -> digest.update(buffer, 0, bytesRead) }
        return digest.digest().joinToString("") { "%02x".format(it) }
    }
}
