package app.organicmaps.widget.colorpicker

import android.content.DialogInterface
import android.content.res.Configuration
import android.graphics.Color
import android.graphics.drawable.Drawable
import android.graphics.drawable.GradientDrawable
import android.graphics.drawable.LayerDrawable
import android.os.Bundle
import android.util.TypedValue
import android.view.Gravity
import android.view.LayoutInflater
import android.view.View
import android.view.View.MeasureSpec
import android.view.ViewGroup
import android.view.ViewGroup.LayoutParams.MATCH_PARENT
import android.view.ViewGroup.LayoutParams.WRAP_CONTENT
import android.view.WindowManager
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.PopupWindow
import android.widget.TextView
import android.widget.ViewFlipper
import androidx.annotation.ColorInt
import androidx.appcompat.content.res.AppCompatResources
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.isVisible
import androidx.fragment.app.viewModels
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.lifecycleScope
import androidx.lifecycle.repeatOnLifecycle
import app.organicmaps.R
import app.organicmaps.util.ThemeUtils
import com.google.android.material.bottomsheet.BottomSheetBehavior
import com.google.android.material.bottomsheet.BottomSheetDialogFragment
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.flow.distinctUntilChanged
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.launch

class TrackColorPickerFragment : BottomSheetDialogFragment() {

    fun interface OnTrackColorChangeListener {
        fun onTrackColorSet(@ColorInt color: Int)
    }

    private val viewModel by viewModels<TrackColorPickerViewModel>()

    private var contentContainer: LinearLayout? = null
    private var viewFlipper: ViewFlipper? = null
    private var divider: View? = null
    private var presetsSection: ViewGroup? = null
    private var gridView: ColorGridView? = null
    private var spectrumView: SpectrumColorView? = null
    private var slidersView: ColorSlidersView? = null
    private var preview: ImageView? = null
    private var presetsContainer: ViewGroup? = null
    private var activePopup: PopupWindow? = null
    private var layoutChangeListener: View.OnLayoutChangeListener? = null
    private var lastBottomInset = 0

    private val circleSize by lazy { resources.getDimensionPixelSize(R.dimen.color_preset_circle_size) }
    private val presetMargin by lazy { resources.getDimensionPixelSize(R.dimen.margin_half_plus) }
    private val selectorStroke by lazy { resources.getDimensionPixelSize(R.dimen.color_picker_selector_stroke) }

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View =
        inflater.inflate(R.layout.fragment_track_color_picker, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        contentContainer = view.findViewById(R.id.content_container)
        viewFlipper = view.findViewById(R.id.view_flipper)
        divider = view.findViewById(R.id.divider)
        presetsSection = view.findViewById(R.id.presets_section)
        gridView = view.findViewById(R.id.grid_view)
        spectrumView = view.findViewById(R.id.spectrum_view)
        slidersView = view.findViewById(R.id.sliders_view)
        preview = view.findViewById(R.id.color_preview)
        presetsContainer = view.findViewById(R.id.presets_container)

        val colorChangedListener = { color: Int -> viewModel.selectColor(color) }
        gridView?.onColorChangedListener = ColorGridView.OnColorChangedListener(colorChangedListener)
        spectrumView?.onColorChangedListener = SpectrumColorView.OnColorChangedListener(colorChangedListener)
        slidersView?.onColorChangedListener = ColorSlidersView.OnColorChangedListener(colorChangedListener)

        val segmentedControl = view.findViewById<PillSegmentedControl>(R.id.tab_toggle)
        segmentedControl.setSegments(
            getString(R.string.color_picker_grid),
            getString(R.string.color_picker_spectrum),
            getString(R.string.color_picker_sliders),
        )
        segmentedControl.selectSegment(viewModel.state.value.activeTab.ordinal)
        segmentedControl.onSegmentSelectedListener = PillSegmentedControl.OnSegmentSelectedListener { index ->
            viewModel.setActiveTab(ColorPickerTab.entries[index.coerceIn(ColorPickerTab.entries.indices)])
        }

        view.findViewById<View>(R.id.btn_close).setOnClickListener { dismiss() }

        updateLayoutForOrientation(resources.configuration.orientation == Configuration.ORIENTATION_LANDSCAPE)

        observeViewModel()
    }

    private fun observeViewModel() {
        viewLifecycleOwner.lifecycleScope.launch {
            viewLifecycleOwner.repeatOnLifecycle(Lifecycle.State.STARTED) {
                launch {
                    viewModel.state.map { it.currentColor }
                        .distinctUntilChanged()
                        .collectLatest { color ->
                            updatePreview(color)
                            gridView?.setColor(color)
                            spectrumView?.setColor(color)
                            slidersView?.setColor(color)
                        }
                }
                launch {
                    viewModel.state.map { it.activeTab }
                        .distinctUntilChanged()
                        .collectLatest { tab ->
                            viewFlipper?.displayedChild = tab.ordinal
                        }
                }
                launch {
                    var prev: ColorPickerState? = null
                    viewModel.state
                        .distinctUntilChanged { old, new ->
                            old.presetColors == new.presetColors && old.selectedPresetIndex == new.selectedPresetIndex
                        }
                        .collect { state ->
                            if (prev?.presetColors != state.presetColors) {
                                rebuildPresetViews(state)
                            } else {
                                updatePresetSelection(prev?.selectedPresetIndex ?: -1, state)
                            }
                            prev = state
                        }
                }
            }
        }
    }

    override fun onDestroyView() {
        activePopup?.dismiss()
        activePopup = null
        layoutChangeListener = null
        contentContainer = null
        viewFlipper = null
        divider = null
        presetsSection = null
        gridView = null
        spectrumView = null
        slidersView = null
        preview = null
        presetsContainer = null
        super.onDestroyView()
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        val isLandScape = newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE
        updateLayoutForOrientation(isLandScape)
        setupBottomSheet()
    }

    override fun onStart() {
        super.onStart()
        setupBottomSheet()
    }

    private fun setupBottomSheet() {
        val sheet = requireView().parent as? View ?: return
        BottomSheetBehavior.from(sheet).apply {
            state = BottomSheetBehavior.STATE_EXPANDED
            skipCollapsed = true
            maxWidth = MATCH_PARENT
        }
        sheet.layoutParams.height = MATCH_PARENT
        sheet.layoutParams.width = MATCH_PARENT
        sheet.requestLayout()

        dialog?.window?.setLayout(MATCH_PARENT, MATCH_PARENT)
        dialog?.window?.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN)

        ViewCompat.setOnApplyWindowInsetsListener(sheet) { v, insets ->
            val bars = insets.getInsets(
                WindowInsetsCompat.Type.systemBars() or WindowInsetsCompat.Type.displayCutout(),
            )
            v.setPadding(bars.left, 0, bars.right, 0)
            lastBottomInset = bars.bottom + resources.getDimensionPixelSize(R.dimen.margin_half)
            applyContentInsets()
            insets
        }
        if (layoutChangeListener == null) {
            val listener = View.OnLayoutChangeListener { v, _, _, _, _, _, _, _, _ ->
                if (v.paddingBottom != 0) {
                    v.setPadding(v.paddingLeft, 0, v.paddingRight, 0)
                }
            }
            layoutChangeListener = listener
            sheet.addOnLayoutChangeListener(listener)
        }
    }

    private fun updateLayoutForOrientation(isLandscape: Boolean) {
        val container = contentContainer ?: return
        val flipper = viewFlipper ?: return
        val div = divider ?: return
        val presets = presetsSection ?: return

        val childHeight = if (isLandscape) MATCH_PARENT else WRAP_CONTENT
        for (i in 0 until flipper.childCount) {
            flipper.getChildAt(i).layoutParams.height = childHeight
        }

        slidersView?.setCompact(isLandscape)

        if (isLandscape) {
            container.orientation = LinearLayout.HORIZONTAL

            flipper.layoutParams = LinearLayout.LayoutParams(0, MATCH_PARENT, 1f)

            div.layoutParams = LinearLayout.LayoutParams(1, MATCH_PARENT).apply {
                topMargin = resources.getDimensionPixelSize(R.dimen.margin_half)
                bottomMargin = resources.getDimensionPixelSize(R.dimen.margin_half)
            }

            presets.layoutParams = LinearLayout.LayoutParams(0, MATCH_PARENT, 1f).apply {
                topMargin = 0
            }
        } else {
            container.orientation = LinearLayout.VERTICAL

            flipper.layoutParams = LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT)

            val margin = resources.getDimensionPixelSize(R.dimen.margin_base)
            div.layoutParams = LinearLayout.LayoutParams(MATCH_PARENT, 1).apply {
                marginStart = margin
                marginEnd = margin
                topMargin = margin
            }

            presets.layoutParams = LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT).apply {
                topMargin = margin
            }
        }

        container.requestLayout()
        applyContentInsets()
    }

    private fun applyContentInsets() {
        val bottom = lastBottomInset
        val flipper = viewFlipper ?: return
        val container = presetsContainer ?: return
        val isLandscape = resources.configuration.orientation == Configuration.ORIENTATION_LANDSCAPE
        if (isLandscape) {
            flipper.setPaddingRelative(flipper.paddingStart, flipper.paddingTop, flipper.paddingEnd, bottom)
            container.setPaddingRelative(container.paddingStart, container.paddingTop, container.paddingEnd, bottom)
        } else {
            flipper.setPaddingRelative(flipper.paddingStart, flipper.paddingTop, flipper.paddingEnd, 0)
            container.setPaddingRelative(container.paddingStart, container.paddingTop, container.paddingEnd, bottom)
        }
    }

    override fun onDismiss(dialog: DialogInterface) {
        if (isResumed) {
            viewModel.getResultColor()?.let {
                (parentFragment as? OnTrackColorChangeListener)?.onTrackColorSet(it)
            }
        }
        super.onDismiss(dialog)
    }

    private fun updatePreview(@ColorInt color: Int) {
        val p = preview ?: return
        val size = resources.getDimensionPixelSize(R.dimen.color_preview_square_size)
        p.setImageDrawable(drawRoundedSquare(color, size, size * 0.2f))
    }

    private fun rebuildPresetViews(state: ColorPickerState) {
        val container = presetsContainer ?: return
        container.removeAllViews()

        state.presetColors.forEachIndexed { i, color ->
            val circle = ImageView(requireContext()).apply {
                tag = i
                layoutParams = ViewGroup.MarginLayoutParams(circleSize, circleSize).apply {
                    marginStart = presetMargin
                    bottomMargin = presetMargin
                }
                setImageDrawable(
                    if (i == state.selectedPresetIndex) {
                        drawCircleWithRing(color, circleSize, selectorStroke)
                    } else {
                        drawFilledCircle(color, circleSize)
                    },
                )
                setOnClickListener {
                    viewModel.selectPreset(i)
                }
                setOnLongClickListener { v ->
                    showDeletePresetPopup(v, i)
                    true
                }
            }
            container.addView(circle)
        }

        container.addView(
            createAddButton(circleSize, presetMargin).apply {
                tag = ADD_BUTTON_TAG
                isVisible = viewModel.canAddPreset()
            },
        )
    }

    private fun updatePresetSelection(oldIndex: Int, state: ColorPickerState) {
        if (oldIndex in state.presetColors.indices) {
            presetsContainer?.findViewWithTag<ImageView>(oldIndex)
                ?.setImageDrawable(drawFilledCircle(state.presetColors[oldIndex], circleSize))
        }
        if (state.selectedPresetIndex in state.presetColors.indices) {
            presetsContainer?.findViewWithTag<ImageView>(state.selectedPresetIndex)
                ?.setImageDrawable(
                    drawCircleWithRing(
                        state.presetColors[state.selectedPresetIndex],
                        circleSize,
                        selectorStroke,
                    ),
                )
        }
        presetsContainer?.findViewWithTag<View>(ADD_BUTTON_TAG)?.isVisible = viewModel.canAddPreset()
    }

    private fun createAddButton(size: Int, margin: Int): ImageView = ImageView(requireContext()).apply {
        layoutParams = ViewGroup.MarginLayoutParams(size, size).apply {
            marginStart = margin
            bottomMargin = margin
        }
        setImageDrawable(createAddButtonDrawable(size))
        setOnClickListener { viewModel.addCurrentColorToPresets() }
    }

    private fun createAddButtonDrawable(size: Int): Drawable {
        val bg = GradientDrawable().apply {
            shape = GradientDrawable.OVAL
            setColor(ThemeUtils.getColor(requireContext(), android.R.attr.colorControlNormal))
            setSize(size, size)
        }

        val plus = AppCompatResources.getDrawable(requireContext(), R.drawable.ic_plus)

        val iconSize = resources.getDimensionPixelSize(R.dimen.margin_base)
        val inset = (size - iconSize) / 2
        return LayerDrawable(arrayOf(bg, plus)).apply {
            setLayerInset(1, inset, inset, inset, inset)
        }
    }

    private fun showDeletePresetPopup(anchor: View, index: Int) {
        activePopup?.dismiss()
        val ctx = requireContext()
        val marginBase = resources.getDimensionPixelSize(R.dimen.margin_base)
        val marginHalf = resources.getDimensionPixelSize(R.dimen.margin_half)

        val label = TextView(ctx).apply {
            text = getString(R.string.delete)
            setTextColor(Color.RED)
            setTextSize(TypedValue.COMPLEX_UNIT_SP, 14f)
            setPadding(marginBase, marginHalf, marginBase, marginHalf)
            background = GradientDrawable().apply {
                setColor(ThemeUtils.getColor(ctx, R.attr.windowBackgroundForced))
                cornerRadius = resources.getDimension(R.dimen.corner_radius_large)
            }
        }

        val popup = PopupWindow(label, WRAP_CONTENT, WRAP_CONTENT, true).apply {
            elevation = resources.getDimension(R.dimen.color_picker_popup_elevation)
            isOutsideTouchable = true
        }
        activePopup = popup

        label.setOnClickListener {
            viewModel.removePreset(index)
            popup.dismiss()
        }

        label.measure(
            MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED),
            MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED),
        )
        val xOff = (anchor.width - label.measuredWidth) / 2
        val yOff = -(label.measuredHeight + anchor.height + marginHalf)
        if (!anchor.isAttachedToWindow) return
        popup.showAsDropDown(anchor, xOff, yOff, Gravity.START)
    }

    companion object {
        private const val ADD_BUTTON_TAG = -1

        private fun drawRoundedSquare(@ColorInt color: Int, size: Int, radius: Float): Drawable =
            GradientDrawable().apply {
                shape = GradientDrawable.RECTANGLE
                setColor(color)
                cornerRadius = radius
                setSize(size, size)
            }

        private fun drawFilledCircle(@ColorInt color: Int, size: Int): Drawable = GradientDrawable().apply {
            shape = GradientDrawable.OVAL
            setColor(color)
            setSize(size, size)
        }

        private fun drawCircleWithRing(@ColorInt color: Int, size: Int, selectorStroke: Int): Drawable {
            val inset = selectorStroke * 2
            return LayerDrawable(
                arrayOf(
                    GradientDrawable().apply {
                        shape = GradientDrawable.OVAL
                        setStroke(selectorStroke, color)
                        setSize(size, size)
                    },
                    GradientDrawable().apply {
                        shape = GradientDrawable.OVAL
                        setColor(color)
                        setSize(size - inset * 2, size - inset * 2)
                    },
                ),
            ).apply {
                setLayerInset(1, inset, inset, inset, inset)
            }
        }
    }
}
