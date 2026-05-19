package app.organicmaps.widget.placepage.sections

import android.annotation.SuppressLint
import android.app.KeyguardManager
import android.content.Context
import android.os.Build
import android.text.Layout
import android.text.Spannable
import android.text.StaticLayout
import android.text.method.LinkMovementMethod
import android.text.style.ClickableSpan
import android.text.util.Linkify
import android.transition.TransitionManager
import android.util.AttributeSet
import android.view.GestureDetector
import android.view.LayoutInflater
import android.view.MotionEvent
import android.view.View
import android.view.ViewStub
import android.webkit.WebView
import android.webkit.WebViewClient
import android.widget.LinearLayout
import android.widget.TextView
import androidx.annotation.ColorInt
import androidx.core.content.ContextCompat
import androidx.core.net.toUri
import androidx.core.text.HtmlCompat
import androidx.core.view.isVisible
import app.organicmaps.R
import app.organicmaps.sdk.util.StringUtils
import app.organicmaps.sdk.util.log.Logger
import app.organicmaps.util.UiUtils
import app.organicmaps.util.Utils
import java.util.Locale
import kotlin.math.roundToInt
import org.json.JSONException
import org.json.JSONTokener

@SuppressLint("ClickableViewAccessibility")
class ExpandableNotesView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0,
) : LinearLayout(context, attrs, defStyleAttr) {
    private val tvNotes: TextView
    private val fadeOverlay: View
    private val moreRegion: TextView

    private val compactHtmlPx: Int = resources.getDimensionPixelSize(R.dimen.notes_html_compact_height)

    private var wvNotes: WebView? = null
    private var isHtml = false
    private var isExpanded = false
    private var canExpand = false

    private var pendingPlainNotes: CharSequence? = null

    // Guards against redundant re-renders when LiveData re-emits the same description
    // (observer re-attach on app foreground, etc.) — preserves isExpanded and avoids WebView reload.
    private var lastNotes: String? = null

    private val webViewTapDetector by lazy {
        GestureDetector(
            context,
            object : GestureDetector.SimpleOnGestureListener() {
                override fun onSingleTapUp(e: MotionEvent): Boolean {
                    handleWebViewTap(e)
                    return true
                }
            },
        )
    }

    private val plainTextTapDetector by lazy {
        GestureDetector(
            context,
            object : GestureDetector.SimpleOnGestureListener() {
                override fun onSingleTapUp(e: MotionEvent): Boolean {
                    if (!isTapOnSpan(tvNotes, e)) toggleExpanded()
                    return false
                }
            },
        )
    }

    init {
        orientation = VERTICAL
        LayoutInflater.from(context).inflate(R.layout.place_page_notes_widget, this)
        tvNotes = findViewById(R.id.tv__notes)
        fadeOverlay = findViewById(R.id.notes_fade_overlay)
        moreRegion = findViewById(R.id.notes_more_region)

        tvNotes.maxLines = COLLAPSED_LINES
        tvNotes.setOnTouchListener { _, event ->
            plainTextTapDetector.onTouchEvent(event)
            false
        }
        tvNotes.setOnLongClickListener {
            copyDescriptionToClipboard()
            true
        }
        moreRegion.setOnClickListener { toggleExpanded() }
        setOnClickListener { toggleExpanded() }
    }

    fun setNotes(notes: String?) {
        if (notes == lastNotes) return
        lastNotes = notes
        when {
            notes.isNullOrEmpty() -> clearInternal()

            StringUtils.nativeIsHtml(notes) -> {
                // Simple HTML (inline tags, basic block elements) renders in the TextView path
                // via Html.fromHtml — synchronous measurement avoids the WebView + anchor jump
                // for the common case. Tables / images / custom CSS still go through WebView.
                if (isSimpleHtml(notes)) {
                    val spanned = HtmlCompat.fromHtml(notes, HtmlCompat.FROM_HTML_MODE_COMPACT)
                    setPlainTextInternal(spanned, autoLinkify = false)
                } else {
                    setHtmlInternal(notes)
                }
            }

            else -> setPlainTextInternal(notes, autoLinkify = true)
        }
    }

    private fun isSimpleHtml(html: String): Boolean {
        // Html.fromHtml doesn't render these meaningfully — needs the WebView path.
        // Lists (ul/ol/li) are included because their bullet/indent rendering is
        // inconsistent across Android versions.
        val complexTags = Regex(
            """<(table|tr|td|th|img|iframe|video|audio|svg|canvas|style|object|embed|ul|ol|li|dl|dt|dd)\b""",
            RegexOption.IGNORE_CASE,
        )
        if (complexTags.containsMatchIn(html)) return false
        // Inline style attributes (color, background, fonts) are also dropped by Html.fromHtml.
        return !Regex("""\bstyle\s*=""", RegexOption.IGNORE_CASE).containsMatchIn(html)
    }

    override fun onDetachedFromWindow() {
        wvNotes?.destroy()
        wvNotes = null
        super.onDetachedFromWindow()
    }

    private fun setPlainTextInternal(notes: CharSequence, autoLinkify: Boolean) {
        hideWebView()
        isVisible = true
        isHtml = false
        isExpanded = false
        canExpand = false

        setMoreOverlayVisibility(GONE)

        tvNotes.apply {
            maxLines = COLLAPSED_LINES
            ellipsize = null
            text = notes
            if (autoLinkify) {
                // Plain text: detect URLs/emails/phones and add ClickableSpans.
                Linkify.addLinks(this, Linkify.WEB_URLS or Linkify.EMAIL_ADDRESSES or Linkify.PHONE_NUMBERS)
            } else {
                // Spanned from Html.fromHtml already has URLSpan for <a> — just wire movement.
                movementMethod = LinkMovementMethod.getInstance()
            }
        }
        UiUtils.show(tvNotes)

        pendingPlainNotes = notes
        requestLayout()
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val pending = pendingPlainNotes
        if (pending != null && MeasureSpec.getMode(widthMeasureSpec) != MeasureSpec.UNSPECIFIED) {
            val parentWidth = MeasureSpec.getSize(widthMeasureSpec)
            val tvWidth = parentWidth - paddingLeft - paddingRight -
                tvNotes.paddingLeft - tvNotes.paddingRight
            if (tvWidth > 0) {
                canExpand = measureFullLineCount(pending, tvWidth) > COLLAPSED_LINES
                setMoreOverlayVisibility(if (canExpand && !isExpanded) VISIBLE else GONE)
                pendingPlainNotes = null
            }
        }
        super.onMeasure(widthMeasureSpec, heightMeasureSpec)
    }

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        if (isHtml || oldw == 0 || w == oldw) return
        val text = tvNotes.text
        if (!text.isNullOrEmpty()) {
            pendingPlainNotes = text
            requestLayout()
        }
    }

    private fun setHtmlInternal(html: String) {
        pendingPlainNotes = null
        UiUtils.hide(tvNotes)
        isVisible = true
        isHtml = true
        isExpanded = false
        canExpand = false

        val webView = ensureWebView()
        setWebViewHeight(compactHtmlPx)
        UiUtils.show(webView)
        // Reserve moreRegion space from the start so the async measureContentHeight callback
        // doesn't grow the widget by ~24dp and shift the bottom-sheet anchor. Stays INVISIBLE
        // (not VISIBLE) if measurement reports short content.
        setMoreOverlayVisibility(INVISIBLE)

        val themedHtml = wrapWithThemeStyles(html)
        webView.loadDataWithBaseURL(null, themedHtml, Utils.TEXT_HTML, "UTF-8", null)
    }

    private fun clearInternal() {
        pendingPlainNotes = null
        UiUtils.hide(tvNotes)
        hideWebView()
        setMoreOverlayVisibility(GONE)
        isVisible = false
        isHtml = false
        isExpanded = false
        canExpand = false
    }

    // Inflated lazily from ViewStub on the first complex-HTML description — descriptions without
    // complex HTML (tables, images, lists, inline CSS) stay WebView-free (~10-20 MB saved per open).
    // JS is required so we can read document.documentElement.scrollHeight via evaluateJavascript()
    // after onPageFinished. The CSP <meta> injected in wrapWithThemeStyles blocks every <script>,
    // javascript: URL and on*-handler that the bookmark/track description could contain, so
    // user-supplied HTML can't execute code; evaluateJavascript() from native bypasses CSP, so
    // measurement still works.
    @SuppressLint("SetJavaScriptEnabled", "ClickableViewAccessibility")
    private fun ensureWebView(): WebView {
        wvNotes?.let { return it }
        findViewById<ViewStub>(R.id.wv__notes_stub).inflate()
        return findViewById<WebView>(R.id.wv__notes).apply {
            setBackgroundColor(ContextCompat.getColor(context, R.color.bg_cards))
            settings.javaScriptEnabled = true
            settings.defaultTextEncodingName = "UTF-8"
            overScrollMode = OVER_SCROLL_NEVER
            isVerticalScrollBarEnabled = false
            isHorizontalScrollBarEnabled = false
            isClickable = false
            isFocusable = false
            setOnTouchListener { _, event ->
                webViewTapDetector.onTouchEvent(event) || !isExpanded
            }
            webViewClient = object : WebViewClient() {
                override fun onPageFinished(view: WebView, url: String?) = measureContentHeight(view)
            }
            wvNotes = this
        }
    }

    private fun measureContentHeight(webView: WebView) {
        if (!isHtml || isExpanded) return
        webView.evaluateJavascript("document.documentElement.scrollHeight") { result ->
            val isCallbackStale =
                wvNotes !== webView || !isHtml || isExpanded || !isAttachedToWindow
            if (isCallbackStale) return@evaluateJavascript
            val cssPx = result.toIntOrNull() ?: return@evaluateJavascript
            val contentPx = (cssPx * resources.displayMetrics.density).roundToInt()
            // Don't shrink the WebView for short content: that would still cause a layout shift
            // even though moreRegion space is reserved (the WebView itself would change height).
            // Just flip the overlay to VISIBLE if overflow, leave INVISIBLE-reserved if not.
            when {
                contentPx > compactHtmlPx -> {
                    canExpand = true
                    setMoreOverlayVisibility(VISIBLE)
                }

                contentPx > 0 -> {
                    canExpand = false
                }
            }
        }
    }

    private fun hideWebView() {
        val wv = wvNotes ?: return
        UiUtils.hide(wv)
        // Release the previously rendered DOM so it doesn't hold ~1-2 MB until the widget is destroyed.
        wv.loadUrl("about:blank")
    }

    private fun toggleExpanded() {
        if (!canExpand) return
        TransitionManager.beginDelayedTransition(this)
        isExpanded = !isExpanded
        if (isHtml) {
            setWebViewHeight(if (isExpanded) LayoutParams.WRAP_CONTENT else compactHtmlPx)
        } else {
            tvNotes.maxLines = if (isExpanded) Int.MAX_VALUE else COLLAPSED_LINES
        }
        setMoreOverlayVisibility(if (isExpanded) GONE else VISIBLE)
    }

    private fun setWebViewHeight(px: Int) {
        val wv = wvNotes ?: return
        if (wv.layoutParams.height == px) return
        wv.layoutParams = wv.layoutParams.apply { height = px }
    }

    private fun setMoreOverlayVisibility(visibility: Int) {
        moreRegion.visibility = visibility
        fadeOverlay.visibility = visibility
    }

    private fun handleWebViewTap(event: MotionEvent) {
        val wv = wvNotes ?: run {
            toggleExpanded()
            return
        }
        val density = resources.displayMetrics.density
        val cssX = event.x / density
        val cssY = event.y / density
        wv.evaluateJavascript(
            """
            (function(){
              var el = document.elementFromPoint($cssX, $cssY);
              while (el) { if (el.tagName === 'A' && el.href) return el.href; el = el.parentElement; }
              return null;
            })()
            """.trimIndent(),
        ) { result ->
            if (wvNotes !== wv || !isHtml || !isAttachedToWindow) return@evaluateJavascript
            val href = parseJsString(result)
            if (href.isNullOrEmpty()) {
                toggleExpanded()
            } else {
                Utils.openUri(context, href.toUri(), R.string.browser_not_available)
            }
        }
    }

    private fun parseJsString(jsResult: String?): String? {
        if (jsResult.isNullOrEmpty() || jsResult == "null") return null
        return try {
            JSONTokener(jsResult).nextValue() as? String
        } catch (e: JSONException) {
            Logger.w(TAG, "Failed to parse JS string result", e)
            null
        }
    }

    private fun copyDescriptionToClipboard() {
        val text = tvNotes.text?.toString().orEmpty()
        if (text.isEmpty()) return
        Utils.copyTextToClipboard(context, text)
        val keyguard = context.getSystemService(Context.KEYGUARD_SERVICE) as KeyguardManager
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU || keyguard.isDeviceLocked) {
            val anchor = rootView.findViewById<View>(R.id.pp_buttons_layout) ?: return
            Utils.showSnackbarAbove(anchor, this, context.getString(R.string.copied_to_clipboard, text))
        }
    }

    private fun isTapOnSpan(view: TextView, event: MotionEvent): Boolean {
        val text = view.text as? Spannable ?: return false
        val layout = view.layout ?: return false
        val x = (event.x - view.totalPaddingLeft + view.scrollX).toInt()
        val y = (event.y - view.totalPaddingTop + view.scrollY).toInt()
        val line = layout.getLineForVertical(y)
        if (x < layout.getLineLeft(line) || x > layout.getLineRight(line)) return false
        val offset = layout.getOffsetForHorizontal(line, x.toFloat())
        return text.getSpans(offset, offset, ClickableSpan::class.java).isNotEmpty()
    }

    @Suppress("DEPRECATION")
    private fun measureFullLineCount(text: CharSequence, width: Int): Int =
        StaticLayout(text, tvNotes.paint, width, Layout.Alignment.ALIGN_NORMAL, 1f, 0f, false).lineCount

    private fun wrapWithThemeStyles(userHtml: String): String = with(tvNotes) {
        val bgCss = toCssColor(ContextCompat.getColor(context, R.color.bg_cards))
        val textCss = toCssColor(currentTextColor)
        val fontSizeDp = (textSize / resources.displayMetrics.density).toInt()
        """<!DOCTYPE html><html><head>""" +
            """<meta http-equiv="Content-Security-Policy" content="script-src 'none'; object-src 'none'">""" +
            """<meta name="viewport" content="width=device-width, initial-scale=1">""" +
            """<meta name="color-scheme" content="light dark">""" +
            """<style>body{background:$bgCss;color:$textCss;font-size:${fontSizeDp}px;margin:0;""" +
            """user-select:none;-webkit-user-select:none}""" +
            """body>:first-child{margin-top:0}body>:last-child{margin-bottom:0}</style>""" +
            """</head><body>$userHtml</body></html>"""
    }

    companion object {
        private const val TAG = "ExpandableNotesView"
        private const val COLLAPSED_LINES = 3

        private fun toCssColor(@ColorInt color: Int): String {
            val a = (color ushr 24) and 0xFF
            val r = (color ushr 16) and 0xFF
            val g = (color ushr 8) and 0xFF
            val b = color and 0xFF
            return String.format(Locale.ROOT, "rgba(%d,%d,%d,%.3f)", r, g, b, a / 255f)
        }
    }
}
