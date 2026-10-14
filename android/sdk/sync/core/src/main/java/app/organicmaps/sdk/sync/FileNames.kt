package app.organicmaps.sdk.sync

/** Category file names, following core GenerateUniqueFileName: Name.kml, Name1.kml, Name2.kml… */
object FileNames {
    private const val KML_EXTENSION = ".kml"
    private val invalidSymbols = Regex("""[\\/:*?"<>|]""")

    /** A name for the local version of a conflicting file, like "Paris (Pixel 6).kml". */
    fun conflictName(fileName: String, device: String, existing: Set<String>): String {
        val baseName = fileName.removeSuffix(KML_EXTENSION)
        return uniqueName("$baseName (${device.replace(invalidSymbols, "")})$KML_EXTENSION", existing)
    }

    fun uniqueName(fileName: String, existing: Set<String>): String {
        if (fileName !in existing) return fileName
        val baseName = fileName.removeSuffix(KML_EXTENSION)
        return generateSequence(1) { it + 1 }.map { "$baseName$it$KML_EXTENSION" }.first { it !in existing }
    }
}
