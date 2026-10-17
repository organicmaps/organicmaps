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
