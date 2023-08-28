package app.organicmaps.help

import android.app.Activity
import androidx.compose.foundation.Image
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material.Divider
import androidx.compose.material.Surface
import androidx.compose.material.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.ColorFilter
import androidx.compose.ui.graphics.painter.Painter
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.dimensionResource
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import app.organicmaps.BuildConfig
import app.organicmaps.R
import app.organicmaps.util.Constants
import app.organicmaps.util.ExtendedMaterialTheme
import app.organicmaps.util.Theme
import app.organicmaps.util.Utils

@Composable
fun HelpItemList(modifier: Modifier = Modifier) {
    val context = LocalContext.current
    val omWebsite = stringResource(id = R.string.translated_om_site_url)

    fun openUrl(url: String) {
        Utils.openUrl(context, url)
    }
    Column(modifier) {
        BoxWithConstraints {
            if (maxWidth < 400.dp) {
                Column {
                    HelpItemsLeft()
                    HelpItemsRight()
                }
            } else {
                Row {
                    HelpItemsLeft(Modifier.weight(1f))
                    HelpItemsRight(Modifier.weight(1f))
                }
            }
        }
        Divider()
        HelpItem(
            text = stringResource(id = R.string.privacy_policy),
            onClick = { openUrl(omWebsite + "privacy/") }
        )
        HelpItem(
            text = stringResource(id = R.string.terms_of_use),
            onClick = { openUrl(omWebsite + "terms/") }
        )
        HelpItem(
            text = stringResource(id = R.string.copyright),
            onClick = {} // TODO migrate that screen to compose as well
        )
    }
}

@Composable
fun HelpItemsLeft(modifier: Modifier = Modifier) {
    val context = LocalContext.current
    val omWebsite = stringResource(id = R.string.translated_om_site_url)
    val telegram = stringResource(id = R.string.telegram_url)

    fun openUrl(url: String) {
        Utils.openUrl(context, url)
    }
    Column(modifier) {
        HelpItem(
            icon = painterResource(id = R.drawable.ic_donate),
            tintIcon = true,
            text = stringResource(id = R.string.how_to_support_us),
            onClick = { Utils.openUrl(context, omWebsite + "support-us/") },
        )
        HelpItem(
            icon = painterResource(id = R.drawable.ic_question_mark),
            tintIcon = true,
            text = stringResource(id = R.string.faq),
            onClick = {} // TODO migrate that screen to compose as well
        )
        HelpItem(
            icon = painterResource(id = R.drawable.ic_news),
            tintIcon = true,
            text = stringResource(id = R.string.news),
            onClick = { openUrl(omWebsite + "news/") },
        )
        HelpItem(
            icon = painterResource(id = R.drawable.ic_rate),
            tintIcon = true,
            text = stringResource(id = R.string.rate_the_app),
            onClick = { Utils.openAppInMarket(context as Activity, BuildConfig.REVIEW_URL) },
        )
        HelpItem(
            icon = painterResource(id = R.drawable.ic_telegram),
            text = stringResource(id = R.string.telegram),
            onClick = { openUrl(telegram) }
        )
        HelpItem(
            icon = painterResource(id = R.drawable.ic_github),
            tintIcon = true,
            text = stringResource(id = R.string.github),
            onClick = { openUrl(Constants.Url.GITHUB) }
        )
        HelpItem(
            icon = painterResource(id = R.drawable.ic_website),
            tintIcon = true,
            text = stringResource(id = R.string.website),
            onClick = { openUrl(omWebsite) }
        )
    }
}


@Composable
fun HelpItemsRight(modifier: Modifier = Modifier) {
    val context = LocalContext.current
    val instagram = stringResource(id = R.string.instagram_url)
    val osm = stringResource(id = R.string.osm_wiki_about_url)

    fun openUrl(url: String) {
        Utils.openUrl(context, url)
    }
    Column(modifier) {
        HelpItem(
            icon = painterResource(id = R.drawable.ic_email),
            tintIcon = true,
            text = stringResource(id = R.string.email),
            onClick = { Utils.sendTo(context, BuildConfig.SUPPORT_MAIL, "Organic Maps") }
        )
        HelpItem(
            icon = painterResource(id = R.drawable.ic_matrix),
            tintIcon = true,
            text = stringResource(id = R.string.matrix),
            onClick = { openUrl(Constants.Url.MATRIX) }
        )
        HelpItem(
            icon = painterResource(id = R.drawable.ic_mastodon),
            text = stringResource(id = R.string.mastodon),
            onClick = { openUrl(Constants.Url.MASTODON) }
        )
        HelpItem(
            icon = painterResource(id = R.drawable.ic_facebook),
            text = stringResource(id = R.string.facebook),
            onClick = { Utils.showFacebookPage(context as Activity) }
        )
        HelpItem(
            icon = painterResource(id = R.drawable.ic_twitterx),
            tintIcon = true,
            text = stringResource(id = R.string.twitter),
            onClick = { openUrl(Constants.Url.TWITTER) }
        )
        HelpItem(
            icon = painterResource(id = R.drawable.ic_instagram),
            text = stringResource(id = R.string.instagram),
            onClick = { openUrl(instagram) }
        )
        HelpItem(
            icon = painterResource(id = R.drawable.ic_openstreetmap),
            tintIcon = true,
            text = stringResource(id = R.string.openstreetmap),
            onClick = { openUrl(osm) }
        )
    }
}


@Composable
fun HelpItem(
    text: String,
    modifier: Modifier = Modifier,
    icon: Painter? = null,
    tintIcon: Boolean = false,
    onClick: () -> Unit = {},
) {
    Box(modifier.clickable { onClick() }) {
        Row(
            verticalAlignment = Alignment.CenterVertically,
            modifier = Modifier
                .height(dimensionResource(id = R.dimen.height_item_oneline))
                .padding(horizontal = dimensionResource(id = R.dimen.margin_base))
                .fillMaxWidth()
        ) {
            if (icon != null) Image(
                icon, contentDescription = "",
                colorFilter = if (tintIcon) ColorFilter.tint(ExtendedMaterialTheme.colors.iconTint) else null
            )
            Text(
                text,
                modifier = if (icon != null) Modifier.padding(start = dimensionResource(id = R.dimen.margin_base_plus)) else Modifier
            )
        }
    }

}

@Preview
@Composable
private fun HelpItemPreview() {
    Theme {
        Surface {
            HelpItem(
                icon = painterResource(id = R.drawable.ic_openstreetmap),
                tintIcon = true,
                text = stringResource(id = R.string.openstreetmap),
                onClick = { /* Nothing */ }
            )
        }
    }
}

@HelpPreviews
@Composable
private fun HelpHeaderPreview() {
    Theme {
        Surface {
            HelpItemList()
        }
    }
}