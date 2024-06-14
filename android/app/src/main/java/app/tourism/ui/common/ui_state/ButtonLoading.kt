import androidx.compose.foundation.layout.size
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

@Composable
fun ButtonLoading() {
    CircularProgressIndicator(
        strokeWidth = 2.dp,
        modifier = Modifier.size(25.dp),
        color = MaterialTheme.colorScheme.background
    )
}