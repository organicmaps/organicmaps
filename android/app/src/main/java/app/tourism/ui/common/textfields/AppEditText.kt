package app.tourism.ui.common.textfields

import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import app.tourism.ui.theme.TextStyles

@Composable
fun AppEditText(
    value: String,
    onValueChange: (String) -> Unit,
    hint: String = "",
    isError: () -> Boolean = { false },
    keyboardOptions: KeyboardOptions = KeyboardOptions.Default,
    keyboardActions: KeyboardActions = KeyboardActions.Default,
    maxLines: Int = 1,
) {
    EditText(
        value = value,
        onValueChange = onValueChange,
        hint = hint,
        hintColor = MaterialTheme.colorScheme.onBackground,
        isError = isError,
        textFieldHeight = 50.dp,
        textFieldPadding = PaddingValues(vertical = 8.dp),
        hintFontSizeInt = 15,
        textSize = 17.sp,
        textStyle = TextStyles.h3.copy(
            textAlign = TextAlign.Start,
            color = MaterialTheme.colorScheme.onBackground,
        ),
        keyboardOptions = keyboardOptions,
        keyboardActions = keyboardActions,
        cursorBrush = SolidColor(MaterialTheme.colorScheme.primary),
        focusedColor = MaterialTheme.colorScheme.onBackground,
        unfocusedColor = MaterialTheme.colorScheme.onBackground,
        errorColor = MaterialTheme.colorScheme.onError,
        maxLines = maxLines,
    )
}
