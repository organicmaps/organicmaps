package app.tourism.ui.common.textfields

import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.VisualTransformation
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

@Composable
fun AuthEditText(
    value: String,
    onValueChange: (String) -> Unit,
    hint: String = "",
    isError: () -> Boolean = { false },
    keyboardOptions: KeyboardOptions = KeyboardOptions.Default,
    keyboardActions: KeyboardActions = KeyboardActions.Default,
    leadingIcon: @Composable (() -> Unit)? = null,
    trailingIcon: @Composable (() -> Unit)? = null,
    visualTransformation: VisualTransformation = VisualTransformation.None
) {
    EditText(
        value = value,
        onValueChange = onValueChange,
        hint = hint,
        hintColor = Color.White,
        isError = isError,
        textFieldHeight = 50.dp,
        textFieldPadding = PaddingValues(vertical = 8.dp),
        hintFontSizeInt = 16,
        textSize = 16.sp,
        textStyle = TextStyle(
            fontWeight = FontWeight.W900,
            textAlign = TextAlign.Start,
            color = Color.White,
        ),
        keyboardOptions = keyboardOptions,
        keyboardActions = keyboardActions,
        cursorBrush = SolidColor(Color.White),
        focusedColor = Color.White,
        unfocusedColor = Color.White,
        errorColor = MaterialTheme.colorScheme.onError,
        leadingIcon = leadingIcon,
        trailingIcon = trailingIcon,
        visualTransformation = visualTransformation
    )
}
