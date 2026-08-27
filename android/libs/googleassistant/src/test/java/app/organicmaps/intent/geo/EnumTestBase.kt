package app.organicmaps.intent.geo

import app.organicmaps.intent.geo.enums.CharEnum
import app.organicmaps.intent.geo.enums.StringEnum
import kotlin.enums.EnumEntries
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

abstract class RawEnumTestBase<T, R>(
    private val enumEntries: EnumEntries<T>,
    private val testedValues: List<Pair<T, R>>,
) where T : Enum<T> {
    @Test
    fun testAllEnumValuesAreDefined() {
        assertEquals(testedValues.size, enumEntries.size)
    }

    @Test
    fun testAllEnumValuesCoveredByMapping() {
        assertEquals(enumEntries.toSet(), testedValues.map { it.first }.toSet())
    }

    @Test
    fun testAllRawValuesCoveredByMapping() {
        assertEquals(enumEntries.map(::rawValue).toSet(), testedValues.map { it.second }.toSet())
    }

    @Test
    fun testAllRawValuesAreUnique() {
        val rawValues = enumEntries.map(::rawValue)
        assertEquals(rawValues.size, rawValues.toSet().size)
    }

    @Test
    fun testRawValueAndFromRaw() {
        testedValues.forEach { (enumValue, rawValue) ->
            assertEquals(rawValue, rawValue(enumValue))
            assertEquals(enumValue, fromRawValue(rawValue))
        }
    }

    protected abstract fun rawValue(enumValue: T): R
    protected abstract fun fromRawValue(rawValue: R): T?
}

abstract class StringEnumTestBase<T>(enumEntries: EnumEntries<T>, testedValues: List<Pair<T, String>>) :
    RawEnumTestBase<T, String>(enumEntries, testedValues) where T : Enum<T>, T : StringEnum {
    @Test
    fun testFromRawNull() {
        assertNull(fromRaw(null))
    }

    @Test
    fun testFromRawInvalid() {
        assertNull(fromRaw("###invalid###"))
    }

    override fun rawValue(enumValue: T): String = enumValue.raw
    override fun fromRawValue(rawValue: String): T? = fromRaw(rawValue)

    protected abstract fun fromRaw(raw: String?): T?
}

abstract class CharEnumTestBase<T>(enumEntries: EnumEntries<T>, testedValues: List<Pair<T, Char>>) :
    RawEnumTestBase<T, Char>(enumEntries, testedValues) where T : Enum<T>, T : CharEnum {
    override fun rawValue(enumValue: T): Char = enumValue.raw
    override fun fromRawValue(rawValue: Char): T? = fromRaw(rawValue)

    @Test
    fun testFromRawInvalid() {
        assertNull(fromRaw('$'))
    }

    protected abstract fun fromRaw(raw: Char): T?
}
