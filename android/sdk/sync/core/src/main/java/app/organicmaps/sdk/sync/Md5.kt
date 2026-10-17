package app.organicmaps.sdk.sync

import java.io.File
import java.security.MessageDigest

/** Lowercase hex MD5 of the file content, as Google Drive reports it in md5Checksum. */
object Md5 {
    fun of(file: File): String {
        val digest = MessageDigest.getInstance("MD5")
        file.inputStream().use { input ->
            val buffer = ByteArray(DEFAULT_BUFFER_SIZE)
            while (true) {
                val read = input.read(buffer)
                if (read < 0) break
                digest.update(buffer, 0, read)
            }
        }
        return digest.digest().joinToString("") { "%02x".format(it) }
    }
}
