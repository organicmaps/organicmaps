package app.organicmaps.sdk.sync

/** Category file names, following core GenerateValidAndUniqueFilePath: Name.kml, Name1.kml, Name2.kml… */
object FileNames {
    const val KML_EXTENSION = ".kml"

    // Core RemoveInvalidSymbols: control characters and characters that some file systems reject.
    private val invalidSymbols = Regex("""[\x00-\x1F\\/:*?"<>|]""")

    // Core kMaxFileNameLength: the name without the extension, in UTF-8 bytes.
    private const val MAX_NAME_BYTES = 200

    /** A name for the local version of a conflicting file, like "Paris (Pixel 6).kml". */
    fun conflictName(fileName: String, device: String, existing: Set<String>): String {
        val name = "${fileName.removeSuffix(KML_EXTENSION)} ($device)".replace(invalidSymbols, "")
        return uniqueName(truncate(name) + KML_EXTENSION, existing)
    }

    fun uniqueName(fileName: String, existing: Set<String>): String {
        if (fileName !in existing) return fileName
        val baseName = fileName.removeSuffix(KML_EXTENSION)
        return generateSequence(1) { it + 1 }.map { "$baseName$it$KML_EXTENSION" }.first { it !in existing }
    }

    private fun truncate(name: String): String {
        var bytes = 0
        var end = 0
        while (end < name.length) {
            val next = name.offsetByCodePoints(end, 1)
            bytes += name.substring(end, next).toByteArray(Charsets.UTF_8).size
            if (bytes > MAX_NAME_BYTES) break
            end = next
        }
        return name.substring(0, end)
    }
}
