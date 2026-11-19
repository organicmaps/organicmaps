package app.organicmaps.syria

import android.app.Activity
import android.graphics.Color
import android.view.LayoutInflater
import android.view.MenuItem
import android.widget.TextView
import androidx.annotation.StringRes
import androidx.appcompat.widget.Toolbar
import app.organicmaps.R
import com.google.android.material.button.MaterialButton
import com.google.android.material.dialog.MaterialAlertDialogBuilder

/**
 * Screen instructions read aloud with the spoken word highlighted (WelcomeNarrator) in a dialog: they open
 * by themselves on the first visit of a screen, and a "?" toolbar icon reopens them.
 */
object NarrationDialog {
    /** The instructions of one screen: dialog title, the message in both languages, its "seen" flag. */
    class Intro(
        @StringRes val title: Int,
        @StringRes val messageAr: Int,
        @StringRes val messageEn: Int,
        val seenKey: String,
    )

    @JvmField
    val LOGIN = Intro(
        R.string.syria_login_intro_title,
        R.string.syria_login_intro_message_ar,
        R.string.syria_login_intro_message_en,
        "login_intro_seen",
    )

    @JvmField
    val RECORDING = Intro(
        R.string.syria_recording_intro_title,
        R.string.syria_recording_intro_message_ar,
        R.string.syria_recording_intro_message_en,
        "recording_intro_seen",
    )

    /** Read before every car navigation: no texting or phone use while driving. */
    @JvmField
    val DRIVE_SAFELY = Intro(
        R.string.syria_drive_safe_title,
        R.string.syria_drive_safe_message_ar,
        R.string.syria_drive_safe_message_en,
        "drive_safe_seen",
    )

    /** Adds the "?" icon to the screen's toolbar and opens the instructions if the screen is new to the user. */
    @JvmStatic
    fun attach(activity: Activity, toolbar: Toolbar, intro: Intro, firstCreate: Boolean) {
        toolbar.menu.add(R.string.syria_login_instructions)
            .setIcon(R.drawable.ic_question_mark)
            .setShowAsActionFlags(MenuItem.SHOW_AS_ACTION_ALWAYS)
            .setOnMenuItemClickListener {
                show(activity, intro)
                true
            }
            .icon?.setTint(Color.WHITE)
        if (firstCreate && !SyriaPrefs.isIntroSeen(activity, intro.seenKey)) show(activity, intro)
    }

    @JvmStatic
    fun show(activity: Activity, intro: Intro) {
        build(activity, intro).setPositiveButton(R.string.syria_close, null).show()
    }

    /** The instructions with a button that goes on, e.g. starts navigation, and one that cancels. */
    @JvmStatic
    fun showThenContinue(activity: Activity, intro: Intro, @StringRes continueText: Int, onContinue: Runnable) {
        build(activity, intro)
            .setPositiveButton(continueText) { _, _ -> onContinue.run() }
            .setNegativeButton(R.string.syria_cancel, null)
            .show()
    }

    private fun build(activity: Activity, intro: Intro): MaterialAlertDialogBuilder {
        val view = LayoutInflater.from(activity).inflate(R.layout.dialog_syria_narration, null)
        val narrator = WelcomeNarrator(
            activity,
            view.findViewById<TextView>(R.id.narration_text),
            view.findViewById<MaterialButton>(R.id.narration_play),
            view.findViewById<MaterialButton>(R.id.narration_language),
            intro.messageAr,
            intro.messageEn,
        )
        return MaterialAlertDialogBuilder(activity)
            .setTitle(intro.title)
            .setView(view)
            .setOnDismissListener {
                narrator.release()
                SyriaPrefs.markIntroSeen(activity, intro.seenKey)
            }
    }
}
