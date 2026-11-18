package app.organicmaps.syria

import android.app.Activity
import android.os.Handler
import android.os.Looper
import android.speech.tts.TextToSpeech
import android.speech.tts.UtteranceProgressListener
import android.text.SpannableString
import android.text.Spanned
import android.text.style.BackgroundColorSpan
import android.view.View
import android.widget.TextView
import android.widget.Toast
import androidx.annotation.StringRes
import androidx.core.graphics.ColorUtils
import app.organicmaps.R
import com.google.android.material.button.MaterialButton
import com.google.android.material.color.MaterialColors
import java.util.Locale

/**
 * Reads a message (by default the first-launch welcome) aloud with the phone's text-to-speech and highlights the word
 * being spoken (the sentence gets a lighter shade, which is all that older Android versions report).
 * Starts by itself in the app language; the language button switches text and voice between Arabic and
 * English.
 */
class WelcomeNarrator @JvmOverloads constructor(
    private val activity: Activity,
    private val textView: TextView,
    private val playButton: MaterialButton,
    private val languageButton: MaterialButton,
    @StringRes private val messageAr: Int = R.string.syria_welcome_message_ar,
    @StringRes private val messageEn: Int = R.string.syria_welcome_message_en,
) : TextToSpeech.OnInitListener {
    private val handler = Handler(Looper.getMainLooper())
    private var tts: TextToSpeech? = TextToSpeech(activity.applicationContext, this)
    private var ready = false
    private var playWhenReady = true
    private var language = SyriaLocale.currentLanguage(activity)
    private var text = ""
    private var sentences: List<IntRange> = emptyList()
    private var speaking = false
    private var onFinished: Runnable? = null

    private val accent = MaterialColors.getColor(textView, android.R.attr.colorAccent)
    private val sentenceColor = ColorUtils.setAlphaComponent(accent, SENTENCE_ALPHA)
    private val wordColor = ColorUtils.setAlphaComponent(accent, WORD_ALPHA)

    init {
        showText()
        playButton.setOnClickListener { if (speaking) stopByUser() else play() }
        languageButton.setOnClickListener { switchLanguage() }
    }

    fun isSpeaking(): Boolean = speaking

    /** Runs [action] when the message has been read to the end, or right away if nothing is playing. */
    fun runWhenFinished(action: Runnable) {
        if (speaking) onFinished = action else action.run()
    }

    /** Goes quiet without running the pending action, e.g. when the screen goes to the background. */
    fun silence() {
        tts?.stop()
        setSpeaking(false)
        clearHighlight()
    }

    fun release() {
        onFinished = null
        handler.removeCallbacksAndMessages(null)
        tts?.stop()
        tts?.shutdown()
        tts = null
        speaking = false
    }

    override fun onInit(status: Int) {
        if (status != TextToSpeech.SUCCESS) {
            playButton.isEnabled = false
            return
        }
        ready = true
        tts?.setOnUtteranceProgressListener(progressListener)
        if (playWhenReady) play()
    }

    private fun play() {
        val engine = tts ?: return
        if (!ready) {
            playWhenReady = true
            return
        }
        playWhenReady = false
        val result = engine.setLanguage(Locale(language))
        if (result == TextToSpeech.LANG_MISSING_DATA || result == TextToSpeech.LANG_NOT_SUPPORTED) {
            val name = activity.getString(if (language == AR) R.string.syria_language_ar else R.string.syria_language_en)
            Toast.makeText(activity, activity.getString(R.string.syria_welcome_no_voice, name), Toast.LENGTH_LONG).show()
            return
        }
        sentences.forEachIndexed { index, range ->
            val mode = if (index == 0) TextToSpeech.QUEUE_FLUSH else TextToSpeech.QUEUE_ADD
            engine.speak(text.substring(range.first, range.last + 1), mode, null, index.toString())
        }
        setSpeaking(true)
    }

    private fun stopByUser() {
        silence()
        finished()
    }

    private fun switchLanguage() {
        tts?.stop()
        setSpeaking(false)
        language = if (language == AR) EN else AR
        showText()
        play()
    }

    private fun showText() {
        text = activity.getString(if (language == AR) messageAr else messageEn)
        sentences = splitSentences(text)
        // The message keeps its own direction whatever the app language.
        textView.layoutDirection = if (language == AR) View.LAYOUT_DIRECTION_RTL else View.LAYOUT_DIRECTION_LTR
        textView.textDirection = if (language == AR) View.TEXT_DIRECTION_RTL else View.TEXT_DIRECTION_LTR
        textView.textAlignment = View.TEXT_ALIGNMENT_VIEW_START
        textView.text = text
        languageButton.setText(if (language == AR) R.string.syria_language_en else R.string.syria_language_ar)
    }

    private fun setSpeaking(value: Boolean) {
        speaking = value
        playButton.setText(if (value) R.string.syria_welcome_stop else R.string.syria_welcome_listen)
        playButton.setIconResource(if (value) R.drawable.ic_voice_off else R.drawable.ic_voice_on)
    }

    private fun finished() {
        setSpeaking(false)
        clearHighlight()
        onFinished?.let {
            onFinished = null
            it.run()
        }
    }

    private fun highlight(sentence: Int, wordStart: Int, wordEnd: Int) {
        val range = sentences.getOrNull(sentence) ?: return
        val spannable = SpannableString(text)
        spannable.setSpan(BackgroundColorSpan(sentenceColor), range.first, range.last + 1, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE)
        if (wordStart in 0 until wordEnd) {
            val start = (range.first + wordStart).coerceAtMost(range.last + 1)
            val end = (range.first + wordEnd).coerceAtMost(range.last + 1)
            spannable.setSpan(BackgroundColorSpan(wordColor), start, end, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE)
        }
        textView.text = spannable
    }

    private fun clearHighlight() {
        textView.text = text
    }

    // Called on a text-to-speech thread: everything is handed to the main thread.
    private val progressListener = object : UtteranceProgressListener() {
        override fun onStart(utteranceId: String) {
            val sentence = utteranceId.toIntOrNull() ?: return
            handler.post { if (speaking) highlight(sentence, -1, -1) }
        }

        override fun onRangeStart(utteranceId: String, start: Int, end: Int, frame: Int) {
            val sentence = utteranceId.toIntOrNull() ?: return
            handler.post { if (speaking) highlight(sentence, start, end) }
        }

        override fun onDone(utteranceId: String) {
            if (utteranceId.toIntOrNull() == sentences.lastIndex) handler.post { if (speaking) finished() }
        }

        @Deprecated("Deprecated in Java")
        override fun onError(utteranceId: String) {
            handler.post { if (speaking) finished() }
        }

        override fun onError(utteranceId: String, errorCode: Int) {
            handler.post { if (speaking) finished() }
        }
    }

    companion object {
        private const val AR = "ar"
        private const val EN = "en"
        private const val SENTENCE_ALPHA = 0x26
        private const val WORD_ALPHA = 0x80

        /** Offsets of each sentence (ending with . ! ? or ؟), trailing spaces left out. */
        @JvmStatic
        fun splitSentences(text: String): List<IntRange> {
            val result = ArrayList<IntRange>()
            Regex("[^.!?؟]+[.!?؟]*").findAll(text).forEach { match ->
                val first = match.range.first + (match.value.length - match.value.trimStart().length)
                val last = match.range.first + match.value.trimEnd().length - 1
                if (last >= first) result.add(first..last)
            }
            return result
        }
    }
}
