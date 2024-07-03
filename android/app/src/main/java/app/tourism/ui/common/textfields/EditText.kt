package app.tourism.ui.common.textfields

import androidx.compose.animation.animateColorAsState
import androidx.compose.animation.core.animateIntAsState
import androidx.compose.animation.core.animateOffsetAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.IntrinsicSize
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.Divider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.onFocusChanged
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.VisualTransformation
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.TextUnit
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlin.math.roundToInt

enum class EtState { Focused, Unfocused, Error }

@Composable
fun EditText(
    value: String,
    onValueChange: (String) -> Unit,
    modifier: Modifier = Modifier,
    hint: String = "",
    hintColor: Color = Color.Gray,
    isError: () -> Boolean = { false },
    errorColor: Color = Color.Red,
    textFieldHeight: Dp = 50.dp,
    textFieldPadding: PaddingValues = PaddingValues(0.dp),
    textSize: TextUnit = 18.sp,
    hintFontSizeInt: Int = 18,
    textStyle: TextStyle = TextStyle(
        fontWeight = FontWeight.Normal,
        color = MaterialTheme.colorScheme.onBackground
    ),
    cursorBrush: Brush = SolidColor(MaterialTheme.colorScheme.primary),
    maxLines: Int = 1,
    keyboardOptions: KeyboardOptions = KeyboardOptions.Default,
    keyboardActions: KeyboardActions = KeyboardActions.Default,
    visualTransformation: VisualTransformation = VisualTransformation.None,
    focusedColor: Color = Color.Green,
    unfocusedColor: Color = Color.Black,
    leadingIcon: @Composable (() -> Unit)? = null,
    trailingIcon: @Composable (() -> Unit)? = null,
) {
    var etState by remember { mutableStateOf(EtState.Unfocused) }

    val hintCondition = etState == EtState.Unfocused && value.isEmpty()
    val hintOffset by animateOffsetAsState(
        targetValue = if (hintCondition) Offset(0f, hintFontSizeInt * 1.6f)
        else Offset(0f, 0f)
    )
    val hintSize by animateIntAsState(
        targetValue = if (hintCondition) hintFontSizeInt else (hintFontSizeInt * 0.8).roundToInt()
    )

    val heightModifier =
        if (maxLines > 1) Modifier.height(IntrinsicSize.Min) else Modifier.height(textFieldHeight)

    Column(modifier) {
        BasicTextField(
            modifier = Modifier
                .padding(textFieldPadding)
                .onFocusChanged {
                    etState = if (it.hasFocus) EtState.Focused else EtState.Unfocused
                }
                .fillMaxWidth()
                .then(heightModifier),
            value = value,
            onValueChange = {
                onValueChange(it)
                etState = if (isError()) EtState.Error else EtState.Focused
            },
            cursorBrush = cursorBrush,
            maxLines = maxLines,
            textStyle = textStyle.copy(
                fontSize = textSize,
                textAlign = TextAlign.Start
            ),
            keyboardOptions = keyboardOptions,
            keyboardActions = keyboardActions,
            visualTransformation = visualTransformation,
            decorationBox = {
                Row(verticalAlignment = Alignment.Bottom) {
                    leadingIcon?.invoke()
                    Column(Modifier.fillMaxSize().weight(1f)) {
                        Text(
                            modifier = Modifier.offset(hintOffset.x.dp, hintOffset.y.dp),
                            text = hint,
                            fontSize = hintSize.sp,
                            color = hintColor,
                        )
                        it()
                    }
                    trailingIcon?.invoke()
                }
            }
        )
        EtLine(etState, focusedColor, unfocusedColor, errorColor)
    }
}

@Composable
fun EtLine(etState: EtState, focusedColor: Color, unfocusedColor: Color, errorColor: Color) {

    val etColor by animateColorAsState(
        targetValue = when (etState) {
            EtState.Focused -> focusedColor
            EtState.Unfocused -> unfocusedColor
            else -> errorColor
        },
        animationSpec = tween(durationMillis = 500), label = "",
    )
    Divider(
        modifier = Modifier.fillMaxWidth(),
        color = etColor,
        thickness = 1.dp
    )
}