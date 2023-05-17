package app.organicmaps.help

import android.app.Activity
import android.text.TextUtils
import androidx.compose.foundation.Image
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.text.selection.SelectionContainer
import androidx.compose.material.Button
import androidx.compose.material.ButtonDefaults
import androidx.compose.material.Icon
import androidx.compose.material.OutlinedButton
import androidx.compose.material.Surface
import androidx.compose.material.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalInspectionMode
import androidx.compose.ui.res.dimensionResource
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import app.organicmaps.BuildConfig
import app.organicmaps.Framework
import app.organicmaps.R
import app.organicmaps.util.Config
import app.organicmaps.util.DateUtils
import app.organicmaps.util.ExtendedMaterialTheme
import app.organicmaps.util.Theme
import app.organicmaps.util.Utils

@Composable
fun HelpHeader() {
    BoxWithConstraints {
        if (maxWidth < 400.dp) {
            Column {
                HelpHeaderLeft()
                HelpHeaderRight()
            }
        } else {
            Row {
                HelpHeaderLeft(Modifier.weight(1f))
                HelpHeaderRight(Modifier.weight(1f))
            }
        }
    }
}

@Composable
fun HelpHeaderLeft(modifier: Modifier = Modifier) {
    Column(modifier, horizontalAlignment = Alignment.CenterHorizontally) {
        Icon(
            painter = painterResource(R.drawable.logo),
            contentDescription = stringResource(id = R.string.app_name),
            tint = ExtendedMaterialTheme.colors.colorLogo,
            modifier = Modifier
                .size(96.dp)
        )
        SelectionContainer {
            Text(
                BuildConfig.VERSION_NAME,
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(vertical = dimensionResource(id = R.dimen.margin_half)),
                textAlign = TextAlign.Center,
                style = ExtendedMaterialTheme.typography.caption
            )
        }
        Text(
            stringResource(id = R.string.about_headline),
            modifier = Modifier
                .fillMaxWidth()
                .padding(bottom = dimensionResource(id = R.dimen.margin_base)),
            textAlign = TextAlign.Center,
            style = ExtendedMaterialTheme.typography.h6
        )

        Text(
            stringResource(id = R.string.about_proposition_1),
            modifier = Modifier
                .fillMaxWidth()
                .padding(bottom = dimensionResource(id = R.dimen.margin_half)),
        )
        Text(
            stringResource(id = R.string.about_proposition_2),
            modifier = Modifier
                .fillMaxWidth()
                .padding(bottom = dimensionResource(id = R.dimen.margin_half)),
        )
        Text(
            stringResource(id = R.string.about_proposition_3),
            modifier = Modifier
                .fillMaxWidth()
                .padding(bottom = dimensionResource(id = R.dimen.margin_half)),
        )
    }
}

@Composable
fun HelpHeaderRight(modifier: Modifier = Modifier) {
    val context = LocalContext.current
    val omWebsite = stringResource(id = R.string.translated_om_site_url)
    // Do not read the config while in the preview
    var donateUrl = if (!LocalInspectionMode.current) Config.getDonateUrl() else ""
    if (TextUtils.isEmpty(donateUrl) && BuildConfig.FLAVOR != "google" && BuildConfig.FLAVOR != "huawei")
        donateUrl = omWebsite + "donate/"

    val dataVersion = if (!LocalInspectionMode.current) DateUtils.getShortDateFormatter()
        .format(Framework.getDataVersion()) else "DATA_VERSION"

    Column(modifier) {
        Text(
            stringResource(id = R.string.about_developed_by_enthusiasts),
            modifier = Modifier
                .fillMaxWidth()
                .padding(bottom = dimensionResource(id = R.dimen.margin_half)),
            fontWeight = FontWeight.Bold
        )
        Row(verticalAlignment = Alignment.CenterVertically) {
            Image(
                painter = painterResource(id = R.drawable.ic_openstreetmap_color),
                contentDescription = stringResource(id = R.string.openstreetmap),
                modifier = Modifier
                    .size(dimensionResource(id = R.dimen.osm_logo))
            )
            Text(
                stringResource(id = R.string.osm_presentation, dataVersion),
                modifier = Modifier.padding(start = dimensionResource(id = R.dimen.margin_half)),
            )
        }
        Button(
            onClick = { Utils.openUrl(context, donateUrl) },
            colors = ButtonDefaults.buttonColors(backgroundColor = ExtendedMaterialTheme.colors.material.secondary),
            modifier = Modifier
                .fillMaxWidth()
                .padding(top = dimensionResource(id = R.dimen.margin_half))
        ) {
            Text(stringResource(id = R.string.donate))
        }
        OutlinedButton(
            onClick = { Utils.sendBugReport(context as Activity, "") },
            colors = ButtonDefaults.buttonColors(backgroundColor = ExtendedMaterialTheme.colors.material.surface),
            modifier = Modifier.fillMaxWidth()
        ) {
            Text(stringResource(id = R.string.report_a_bug))
        }
    }
}

@HelpPreviews
@Composable
private fun HelpHeaderPreview() {
    Theme {
        Surface {
            HelpHeader()
        }
    }
}