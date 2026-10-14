package app.organicmaps.sdk.sync

import org.junit.Assert.assertEquals
import org.junit.Test

class FileNamesTest {
    @Test
    fun conflictName_appendsDevice() {
        assertEquals("Paris (Pixel 6).kml", FileNames.conflictName("Paris.kml", "Pixel 6", emptySet()))
    }

    @Test
    fun conflictName_isUniqueLikeCore() {
        assertEquals(
            "Paris (Pixel 6)1.kml",
            FileNames.conflictName("Paris.kml", "Pixel 6", setOf("Paris (Pixel 6).kml")),
        )
    }

    @Test
    fun conflictName_keepsUnicode() {
        assertEquals("Café ☕ (Pixel6).kml", FileNames.conflictName("Café ☕.kml", "Pixel/6", emptySet()))
    }

    @Test
    fun conflictName_dropsCharactersInvalidInFileNames() {
        assertEquals("A (abcdefghi).kml", FileNames.conflictName("A.kml", "a\\b/c:d*e?f\"g<h>i|", emptySet()))
        assertEquals("A (Pixel6).kml", FileNames.conflictName("A.kml", "Pixel\n6\t", emptySet()))
    }

    @Test
    fun conflictName_truncatesLikeCore() {
        // Core keeps the name without the extension within 200 UTF-8 bytes, cutting at a code point: 195 + " (П" is
        // 199 bytes, and the next two-byte letter does not fit.
        val name = "a".repeat(195) + ".kml"

        assertEquals("a".repeat(195) + " (П.kml", FileNames.conflictName(name, "Пиксель", emptySet()))
    }

    @Test
    fun uniqueName_returnsFreeNameAsIs() {
        assertEquals("Paris.kml", FileNames.uniqueName("Paris.kml", setOf("Rome.kml")))
    }

    @Test
    fun uniqueName_appendsFirstFreeNumber() {
        assertEquals("Paris2.kml", FileNames.uniqueName("Paris.kml", setOf("Paris.kml", "Paris1.kml")))
    }
}
