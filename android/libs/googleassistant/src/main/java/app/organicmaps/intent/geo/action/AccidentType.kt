package app.organicmaps.intent.geo.action

import app.organicmaps.intent.geo.enums.StringEnum

enum class AccidentType(override val raw: String) : StringEnum {
    Minor("minor"),
    Major("major"),
    ;

    companion object {
        const val KEY: String = "accident_type"
    }
}
