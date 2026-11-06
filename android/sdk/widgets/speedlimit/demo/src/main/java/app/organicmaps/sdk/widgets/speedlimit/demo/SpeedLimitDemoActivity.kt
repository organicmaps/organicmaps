package app.organicmaps.sdk.widgets.speedlimit.demo

import android.os.Bundle
import android.widget.Button
import android.widget.SeekBar
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.app.AppCompatDelegate
import androidx.appcompat.widget.SwitchCompat
import app.organicmaps.sdk.widgets.speedlimit.CombinedSpeedViewController
import app.organicmaps.sdk.widgets.speedlimit.Mode
import app.organicmaps.sdk.widgets.speedlimit.SpeedLimitView
import app.organicmaps.sdk.widgets.speedlimit.SpeedThresholds

/**
 * A playground for circular and square speed-limit signs, standalone and combined with current speed.
 * The bars control the speed and speed limit for all signs.
 *
 * Both variants use [SpeedThresholds.DEFAULT] in their displayed units: km/h for circular limits
 * and mph for square limits. The current-speed signs are always square.
 */
class SpeedLimitDemoActivity : AppCompatActivity() {
    private lateinit var speedLabel: TextView
    private lateinit var speedLimitLabel: TextView

    private lateinit var speedLimitCircle: SpeedLimitView
    private lateinit var speedLimitSquare: SpeedLimitView
    private lateinit var controllers: List<CombinedSpeedViewController>

    private var speed = INITIAL_SPEED
    private var speedLimit = INITIAL_SPEED_LIMIT

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_speed_limit_demo)

        speedLabel = findViewById(R.id.speed_label)
        speedLimitLabel = findViewById(R.id.speed_limit_label)

        speedLimitCircle = findViewById(R.id.speed_limit_circle)
        speedLimitSquare = findViewById(R.id.speed_limit_square)

        controllers = listOf(
            CombinedSpeedViewController(findViewById(R.id.combined_circle), SpeedThresholds.DEFAULT),
            CombinedSpeedViewController(findViewById(R.id.combined_square), SpeedThresholds.DEFAULT),
        )
        controllers[0].units = getString(R.string.units_kmh)
        controllers[1].units = getString(R.string.units_mph)

        setUpBar(R.id.speed_bar, speed) { speed = it }
        setUpBar(R.id.speed_limit_bar, speedLimit) { speedLimit = it }

        findViewById<Button>(R.id.show_speed_limit).setOnClickListener {
            controllers.forEach { it.showSpeedLimit() }
        }
        findViewById<Button>(R.id.hide_speed_limit).setOnClickListener {
            controllers.forEach { it.hideSpeedLimit() }
        }

        val darkTheme = findViewById<SwitchCompat>(R.id.dark_theme)
        darkTheme.isChecked = AppCompatDelegate.getDefaultNightMode() == AppCompatDelegate.MODE_NIGHT_YES
        darkTheme.setOnCheckedChangeListener { _, checked ->
            AppCompatDelegate.setDefaultNightMode(
                if (checked) AppCompatDelegate.MODE_NIGHT_YES else AppCompatDelegate.MODE_NIGHT_NO,
            )
        }

        update()
    }

    private inline fun setUpBar(barId: Int, initialValue: Int, crossinline onValue: (Int) -> Unit) {
        findViewById<SeekBar>(barId).apply {
            max = MAX_SPEED
            setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
                override fun onProgressChanged(bar: SeekBar, progress: Int, fromUser: Boolean) {
                    onValue(progress)
                    update()
                }

                override fun onStartTrackingTouch(bar: SeekBar) = Unit

                override fun onStopTrackingTouch(bar: SeekBar) = Unit
            })
            progress = initialValue
        }
    }

    private fun update() {
        speedLabel.text = getString(R.string.value, getString(R.string.speed), speed)
        speedLimitLabel.text = getString(R.string.value, getString(R.string.speed_limit), speedLimit)

        // Standalone signs have no controller, so the value and the mode are set here.
        for (view in listOf(speedLimitCircle, speedLimitSquare)) {
            view.mode = SpeedThresholds.DEFAULT.modeFor(
                speed,
                speedLimit,
                if (view.speedLimit == speedLimit) view.mode else Mode.Normal,
            )
            view.speedLimit = speedLimit
        }

        controllers.forEach { controller ->
            controller.speed = speed
            controller.speedLimit = speedLimit
        }
    }

    private companion object {
        const val MAX_SPEED = 200
        const val INITIAL_SPEED = 60
        const val INITIAL_SPEED_LIMIT = 60
    }
}
