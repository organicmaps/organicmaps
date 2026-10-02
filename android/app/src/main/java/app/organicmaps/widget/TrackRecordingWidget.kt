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

        // Smaller widgets show only the time, distance and button.
        private val COMPACT_SIZE = SizeF(250f, 110f)
        private val FULL_SIZE = SizeF(250f, 180f)

        private var lastContent: TrackRecordingWidgetContent? = null

        // Avoids an IPC on every update when there are no widgets.
        private var hasWidgets: Boolean? = null

        private val mainHandler = Handler(Looper.getMainLooper())
        private var updateScheduled = false

        // The chronometer counts from the time the statistics arrived, not from the render.
        private var statisticsTimeMs = 0L

        @JvmStatic
        fun update(context: Context) {
            statisticsTimeMs = SystemClock.elapsedRealtime()
            render(context, force = false)
        }

        private fun render(context: Context, force: Boolean) {
            // null on devices without app widgets, e.g. Android Automotive.
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

        // Show deferred statistics even if no new ones arrive.
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
            // Whole meters, as on the place page.
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
            val finish = createFinishPendingIntent(context)
            views.setOnClickPendingIntent(R.id.finish, finish)
            views.setOnClickPendingIntent(R.id.widget_root, finish)
            return views
        }

        private fun createFinishPendingIntent(context: Context): PendingIntent {
            val immutable = if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) 0 else PendingIntent.FLAG_IMMUTABLE
            val flags = PendingIntent.FLAG_UPDATE_CURRENT or immutable
            val intent =
                Intent(context, MwmActivity::class.java)
                    .putExtra(MwmActivity.EXTRA_FINISH_TRACK_RECORDING, true)
            // Extras alone don't make a PendingIntent unique.
            return PendingIntent.getActivity(context, 1, intent, flags)
        }

        // No core calls here: this runs at process start, before the core is initialized.
        private fun createIdleViews(context: Context): RemoteViews {
            val views = createViews(context, recording = false)
            // The launcher may reuse the recording views, so stop the chronometer explicitly.
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

        // A stopped Chronometer still shows the time since its base.
        private fun setStoppedTime(views: RemoteViews) {
            views.setTextViewText(R.id.chronometer, DateUtils.formatElapsedTime(0))
        }
    }
}
