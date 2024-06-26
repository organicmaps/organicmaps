package app.tourism.ui.common

import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.width
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.Dp

@Composable
fun HorizontalSpace(width: Dp) = Spacer(modifier = Modifier.width(width))

@Composable
fun VerticalSpace(height: Dp) = Spacer(modifier = Modifier.height(height))