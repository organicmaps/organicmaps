package app.tourism.ui.common

import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.RowScope
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.width
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.Dp

@Composable
fun RowScope.HorizontalSpace(width: Dp) = Spacer(modifier = Modifier.width(width))

@Composable
fun ColumnScope.VerticalSpace(height: Dp) = Spacer(modifier = Modifier.height(height))