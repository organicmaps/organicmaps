package app.organicmaps.help

import android.app.Activity
import android.content.res.Configuration.UI_MODE_NIGHT_YES
import android.os.Bundle
import androidx.activity.compose.setContent
import androidx.appcompat.app.AppCompatActivity
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.Icon
import androidx.compose.material.IconButton
import androidx.compose.material.Scaffold
import androidx.compose.material.Text
import androidx.compose.material.TopAppBar
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.dimensionResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.tooling.preview.Preview
import app.organicmaps.R
import app.organicmaps.util.Theme

class HelpActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            HelpContent()
        }
    }
}

@Composable
private fun HelpContent() {
    val context = LocalContext.current
    Theme {
        Scaffold(
            topBar = {
                TopAppBar(
                    title = {
                        Text(
                            stringResource(id = R.string.about_menu_title)
                        )
                    },
                    navigationIcon = {
                        IconButton(onClick = { (context as Activity).finish() }) {
                            Icon(
                                Icons.Filled.ArrowBack,
                                contentDescription = stringResource(id = R.string.back)
                            )
                        }
                    },
                )
            },
        ) { contentPadding ->
            Box(
                Modifier
                    .padding(contentPadding)
                    .verticalScroll(
                        rememberScrollState()
                    )
            ) {
                Column(
                    Modifier
                        .padding(dimensionResource(id = R.dimen.margin_base))
                ) {
                    Spacer(modifier = Modifier.height(dimensionResource(id = R.dimen.margin_base)))
                    HelpHeader()
                    HelpItemList(Modifier.padding(top = dimensionResource(id = R.dimen.margin_half)))
                }

            }
        }
    }
}

@Preview(
    name = "dark theme",
    uiMode = UI_MODE_NIGHT_YES
)
@Preview(name = "light theme")
@Preview(
    name = "landscape",
    device = "spec:width=700dp,height=300dp,dpi=480,orientation=landscape"
)
annotation class HelpPreviews

@HelpPreviews
@Composable
private fun HelpPreview() {
    HelpContent()
}
