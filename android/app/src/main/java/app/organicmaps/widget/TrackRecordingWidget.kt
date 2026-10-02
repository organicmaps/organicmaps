package app.organicmaps.widget

import android.app.PendingIntent
import android.appwidget.AppWidgetManager
import android.appwidget.AppWidgetProvider
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import android.text.format.DateUtils
import android.util.SizeF
import android.view.View
import android.widget.RemoteViews
import app.organicmaps.MwmActivity
import app.organicmaps.R
import app.organicmaps.location.TrackRecordingService
import app.organicmaps.sdk.Framework
import app.organicmaps.sdk.bookmarks.data.TrackStatistics
import app.organicmaps.sdk.util.StringUtils

/** Home screen widget that starts track recording and shows its statistics. */
class TrackRecordingWidget : AppWidgetProvider() {
    override fun onUpdate(context: Context, appWidgetManager: AppWidgetManager, appWidgetIds: IntArray) {
        render(context, force = true)
    }

    override fun onEnabled(context: Context) {
        hasWidgets = true
    }

    override fun onDisabled(context: Context) {
        hasWidgets = false
    }

    companion object {
        private const val NO_VALUE = "—"
        private val IDLE_TEXTS = TrackRecordingWidgetContent.Texts(NO_VALUE, NO_VALUE, NO_VALUE, NO_VALUE, NO_VALUE)

        // Below this height only the time, distance and button fit, e.g. in two landscape rows.
        private val COMPACT_SIZE = SizeF(250f, 110f)
        private val FULL_SIZE = SizeF(250f, 180f)

        private var lastContent: TrackRecordingWidgetContent? = null

        // Spares an IPC on every location update when no widget is placed.
        private var hasWidgets: Boolean? = null

        private val mainHandler = Handler(Looper.getMainLooper())
        private var updateScheduled = false

        // When the shown statistics arrived: the chronometer counts from them, however late they are rendered.
        private var statisticsTimeMs = 0L

        /** Renders the new recording state or statistics, skipping updates that would not change what is shown. */
        @JvmStatic
        fun update(context: Context) {
            statisticsTimeMs = SystemClock.elapsedRealtime()
            render(context, force = false)
        }

        private fun render(context: Context, force: Boolean) {
            // There is no manager on devices without app widgets, e.g. Android Automotive.
            val manager = AppWidgetManager.getInstance(context) ?: return
            val component = ComponentName(context, TrackRecordingWidget::class.java)
            if (hasWidgets == null) {
                hasWidgets = manager.getAppWidgetIds(component).isNotEmpty()
            }
            if (hasWidgets != true) {
                return
            }

            val views: RemoteViews
            if (TrackRecordingService.isRecording().value == true) {
                val stats = TrackRecordingService.getStatistics().value
                val now = SystemClock.elapsedRealtime()
                val delayMs = TrackRecordingWidgetContent.formatDelayMs(lastContent, stats?.duration ?: 0.0, now)
                if (!force && delayMs > 0) {
                    scheduleUpdate(context, delayMs)
                    return
                }
                val content = createContent(context, stats, now)
                if (!force && !content.needsPush(lastContent)) {
                    return
                }
                lastContent = content
                views = createRecordingViews(context, content)
            } else {
                lastContent = null
                views = createIdleViews(context)
            }
            manager.updateAppWidget(component, withCompactLayout(views))
        }

        private fun withCompactLayout(views: RemoteViews): RemoteViews {
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.S) {
                return views
            }
            val compact = RemoteViews(views)
            compact.setViewVisibility(R.id.header, View.GONE)
            compact.setViewVisibility(R.id.stats, View.GONE)
            return RemoteViews(mapOf(COMPACT_SIZE to compact, FULL_SIZE to views))
        }

        // Shows the statistics held back by the interval even if no new ones arrive meanwhile.
        private fun scheduleUpdate(context: Context, delayMs: Long) {
            if (updateScheduled) {
                return
            }
            updateScheduled = true
            val appContext = context.applicationContext
            mainHandler.postDelayed({
                updateScheduled = false
                render(appContext, force = false)
            }, delayMs)
        }

        private fun createContent(
            context: Context,
            stats: TrackStatistics?,
            elapsedRealtimeMs: Long,
        ): TrackRecordingWidgetContent {
            // Ascent and descent in whole meters, as on the recording Place Page.
            val texts =
                TrackRecordingWidgetContent.Texts(
                    distance = StringUtils.nativeFormatDistance(stats?.length ?: 0.0).toString(context),
                    ascent = formatAltitude((stats?.ascent ?: 0.0).toInt()),
                    descent = formatAltitude((stats?.descent ?: 0.0).toInt()),
                    maxElevation = formatAltitude(stats?.maxElevation ?: 0),
                    minElevation = formatAltitude(stats?.minElevation ?: 0),
                )
            return TrackRecordingWidgetContent.create(
                texts,
                stats?.duration ?: 0.0,
                statisticsTimeMs,
                elapsedRealtimeMs,
            )
        }

        private fun formatAltitude(meters: Int): String = Framework.nativeFormatAltitude(meters.toDouble())

        private fun createRecordingViews(context: Context, content: TrackRecordingWidgetContent): RemoteViews {
            val views = createViews(context, recording = true)
            views.setChronometer(R.id.chronometer, content.chronometerBase, null, content.isTicking)
            if (!content.isTicking) {
                setStoppedTime(views)
            }
            setTexts(views, content.texts)
            // Finish button and a tap on the widget both open the recording place page.
            val finish = createFinishPendingIntent(context)
            views.setOnClickPendingIntent(R.id.finish, finish)
            views.setOnClickPendingIntent(R.id.widget_root, finish)
            return views
        }

        /** Opens the recording place page, where the user saves or deletes the track. */
        private fun createFinishPendingIntent(context: Context): PendingIntent {
            val immutable = if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) 0 else PendingIntent.FLAG_IMMUTABLE
            val flags = PendingIntent.FLAG_UPDATE_CURRENT or immutable
            val intent =
                Intent(context, MwmActivity::class.java)
                    .putExtra(MwmActivity.EXTRA_FINISH_TRACK_RECORDING, true)
            // A request code unlike the content intent's: pending intents differing only in extras are equal.
            return PendingIntent.getActivity(context, 1, intent, flags)
        }

        /** Must not call the core: the widget is also rendered at process start, before the core is initialized. */
        private fun createIdleViews(context: Context): RemoteViews {
            val views = createViews(context, recording = false)
            // The launcher may re-apply these views to the ones of the recording state, so the chronometer is
            // stopped explicitly.
            views.setChronometer(R.id.chronometer, SystemClock.elapsedRealtime(), null, false)
            setStoppedTime(views)
            setTexts(views, IDLE_TEXTS)
            views.setOnClickPendingIntent(R.id.start, TrackRecordingStartActivity.createPendingIntent(context))
            views.setOnClickPendingIntent(R.id.widget_root, TrackRecordingService.createContentPendingIntent(context))
            return views
        }

        private fun createViews(context: Context, recording: Boolean): RemoteViews {
            val views = RemoteViews(context.packageName, R.layout.widget_track_recording)
            views.setViewVisibility(R.id.title_idle, if (recording) View.GONE else View.VISIBLE)
            views.setViewVisibility(R.id.title_recording, if (recording) View.VISIBLE else View.GONE)
            views.setViewVisibility(R.id.start, if (recording) View.GONE else View.VISIBLE)
            views.setViewVisibility(R.id.finish, if (recording) View.VISIBLE else View.GONE)
            return views
        }

        private fun setTexts(views: RemoteViews, texts: TrackRecordingWidgetContent.Texts) {
            views.setTextViewText(R.id.distance, texts.distance)
            views.setTextViewText(R.id.ascent, texts.ascent)
            views.setTextViewText(R.id.descent, texts.descent)
            views.setTextViewText(R.id.max_elevation, texts.maxElevation)
            views.setTextViewText(R.id.min_elevation, texts.minElevation)
        }

        /** A stopped chronometer shows the time since its base, which grows each time the launcher re-applies it. */
        private fun setStoppedTime(views: RemoteViews) {
            views.setTextViewText(R.id.chronometer, DateUtils.formatElapsedTime(0))
        }
    }
}
