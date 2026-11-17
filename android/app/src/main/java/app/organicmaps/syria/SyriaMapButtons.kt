package app.organicmaps.syria

import android.animation.ObjectAnimator
import android.animation.PropertyValuesHolder
import android.animation.ValueAnimator
import android.app.Activity
import android.content.Context
import android.content.ContextWrapper
import android.content.res.ColorStateList
import android.view.View
import android.view.ViewGroup
import android.view.animation.AccelerateDecelerateInterpolator
import android.widget.ImageView
import androidx.annotation.ColorRes
import androidx.core.content.ContextCompat
import androidx.lifecycle.DefaultLifecycleObserver
import androidx.lifecycle.LifecycleOwner
import app.organicmaps.R
import app.organicmaps.syria.tracking.ActivityTrackingService
import app.organicmaps.syria.tracking.TrackingEngine

/**
 * Recording button and internet indicator on the map. The button is a traffic light: green while recording,
 * red otherwise, pulsing gently to invite the user to start a recording.
 */
object SyriaMapButtons {
    @JvmStatic
    @ColorRes
    fun colorOf(state: SyriaStatus.SyncState): Int = when (state) {
        SyriaStatus.SyncState.SYNCED -> R.color.syria_sync_green
        SyriaStatus.SyncState.PENDING, SyriaStatus.SyncState.SYNCING -> R.color.syria_sync_orange
        SyriaStatus.SyncState.FAILED, SyriaStatus.SyncState.SIGNED_OUT -> R.color.syria_sync_red
    }

    /** Binds the views of the regular map buttons layout; the navigation layout has none of them. */
    @JvmStatic
    fun bind(owner: LifecycleOwner, frame: View) {
        frame.findViewById<ImageView>(R.id.syria_recording_button)?.let { button ->
            val pulse = Pulse(button)
            owner.lifecycle.addObserver(pulse)
            renderRecording(button, pulse, null)
            ActivityTrackingService.state().observe(owner) { renderRecording(button, pulse, it?.status) }
        }
        val activity = frame.context.findActivity()
        frame.findViewById<View>(R.id.syria_help_button)?.let { help ->
            if (activity != null) {
                help.setOnClickListener { SyriaTutorial.show(activity, frame) }
                SyriaTutorial.showOnFirstVisits(activity, frame)
            }
        }
        frame.findViewById<ImageView>(R.id.syria_connectivity)?.let { icon ->
            SyriaStatus.online().observe(owner) { renderOnline(icon, it == true) }
        }
    }

    /**
     * Keeps the help + traffic light stack below the top panels: [margin] is where navigation wants the
     * top-end buttons (below the street name), or -1 when no panel is shown.
     */
    @JvmStatic
    fun placeBelowTopPanel(frame: View?, margin: Int, statusButton: View?) {
        val stack = frame?.findViewById<View>(R.id.syria_buttons_stack) ?: return
        val params = stack.layoutParams as? ViewGroup.MarginLayoutParams ?: return
        val defaultTop = frame.resources.getDimensionPixelSize(R.dimen.action_bar_extended_height)
        // Organic Maps' own recording status button sits at that margin when it is shown.
        val status = statusButton?.takeIf { it.visibility == View.VISIBLE }?.height ?: 0
        val top = if (margin < 0) defaultTop else maxOf(defaultTop, margin + status)
        if (params.topMargin == top) return
        params.topMargin = top
        stack.layoutParams = params
    }

    private fun Context.findActivity(): Activity? {
        var context: Context? = this
        while (context is ContextWrapper) {
            if (context is Activity) return context
            context = context.baseContext
        }
        return null
    }

    @JvmStatic
    fun isRecording(status: TrackingEngine.Status?): Boolean = status == TrackingEngine.Status.ACTIVE

    private fun renderRecording(button: ImageView, pulse: Pulse, status: TrackingEngine.Status?) {
        val recording = isRecording(status)
        button.setImageResource(if (recording) R.drawable.ic_syria_lamp_green else R.drawable.ic_syria_lamp_red)
        button.contentDescription = button.context.getString(
            when (status ?: TrackingEngine.Status.IDLE) {
                TrackingEngine.Status.ACTIVE -> R.string.syria_status_active
                TrackingEngine.Status.PAUSED -> R.string.syria_status_paused
                TrackingEngine.Status.IDLE -> R.string.syria_status_idle
            },
        )
        pulse.setEnabled(!recording)
    }

    /** Slow "breathing" of the red light; runs only while the map is visible. */
    private class Pulse(private val view: View) : DefaultLifecycleObserver {
        private var enabled = false
        private var started = false
        private var animator: ObjectAnimator? = null

        fun setEnabled(value: Boolean) {
            enabled = value
            update()
        }

        override fun onStart(owner: LifecycleOwner) {
            started = true
            update()
        }

        override fun onStop(owner: LifecycleOwner) {
            started = false
            update()
        }

        private fun update() {
            if (enabled && started) {
                if (animator != null) return
                animator = ObjectAnimator.ofPropertyValuesHolder(
                    view,
                    PropertyValuesHolder.ofFloat(View.SCALE_X, 1f, PULSE_SCALE),
                    PropertyValuesHolder.ofFloat(View.SCALE_Y, 1f, PULSE_SCALE),
                ).apply {
                    duration = PULSE_MS
                    repeatCount = ValueAnimator.INFINITE
                    repeatMode = ValueAnimator.REVERSE
                    interpolator = AccelerateDecelerateInterpolator()
                    start()
                }
            } else {
                animator?.cancel()
                animator = null
                view.scaleX = 1f
                view.scaleY = 1f
            }
        }
    }

    private const val PULSE_SCALE = 1.12f
    private const val PULSE_MS = 900L

    private fun renderOnline(icon: ImageView, online: Boolean) {
        icon.setImageResource(if (online) R.drawable.ic_syria_online else R.drawable.ic_syria_offline)
        val color = if (online) R.color.syria_sync_green else R.color.syria_sync_red
        icon.imageTintList = ColorStateList.valueOf(ContextCompat.getColor(icon.context, color))
        icon.contentDescription = icon.context.getString(if (online) R.string.syria_online else R.string.syria_offline)
    }
}
