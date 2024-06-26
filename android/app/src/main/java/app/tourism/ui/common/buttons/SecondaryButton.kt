package app.tourism.ui.common.buttons

import ButtonLoading
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.res.colorResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import app.organicmaps.R
import app.tourism.ui.common.HorizontalSpace
import app.tourism.ui.theme.TextStyles

@Composable
fun SecondaryButton(
    modifier: Modifier = Modifier,
    label: String,
    loading: Boolean = false,
    icon: (@Composable () -> Unit)? = null,
    onClick: () -> Unit
) {
    val shape = RoundedCornerShape(16.dp)

    Box(
        modifier = Modifier
            .height(height =  56.dp)
            .background(color = colorResource(id = R.color.transparent), shape = shape)
            .border(width = 1.dp, color = MaterialTheme.colorScheme.primary, shape = shape)
            .clip(shape)
            .clickable { onClick() }
            .then(modifier),
    ) {
        Row(
            modifier = Modifier
                .fillMaxSize()
                .padding(horizontal = 16.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.Center
        ) {
            if (!loading) {
                icon?.apply {
                    invoke()
                    HorizontalSpace(width = 8.dp)
                }
                Text(
                    text = label,
                    style = TextStyles.h4,
                    textAlign = TextAlign.Center,
                    fontSize = 16.sp,
                    fontWeight = FontWeight.W600,
                    color = MaterialTheme.colorScheme.primary
                )
            } else {
                ButtonLoading()
            }
        }
    }
}