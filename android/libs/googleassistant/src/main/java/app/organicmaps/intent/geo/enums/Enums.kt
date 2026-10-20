package app.organicmaps.intent.geo.enums

import kotlin.enums.enumEntries

interface StringEnum {
    val raw: String
}

interface CharEnum {
    val raw: Char
}

object Enums {
    inline fun <reified T> fromRaw(raw: String?): T?
        where T : Enum<T>, T : StringEnum {
        if (raw == null) {
            return null
        }
        return enumEntries<T>().firstOrNull { it.raw == raw }
    }

    inline fun <reified T> fromRaw(raw: Char): T?
        where T : Enum<T>, T : CharEnum =
        enumEntries<T>().firstOrNull { it.raw == raw }
}
