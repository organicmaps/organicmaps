package app.tourism.ui.screens.main.place_details

import androidx.lifecycle.ViewModel
import app.tourism.Constants
import app.tourism.data.dto.PlaceLocation
import app.tourism.domain.models.details.PlaceFull
import app.tourism.utils.makeLongListOfTheSameItem
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.receiveAsFlow
import kotlinx.coroutines.flow.update
import javax.inject.Inject

@HiltViewModel
class PlaceViewModel @Inject constructor(
) : ViewModel() {
    private val uiChannel = Channel<UiEvent>()
    val uiEventsChannelFlow = uiChannel.receiveAsFlow()

    private val _place = MutableStateFlow<PlaceFull?>(null)
    val place = _place.asStateFlow()

    fun setFavoriteChanged(itemId: Long, isFavorite: Boolean) {
        // todo
    }

    init {
        //todo replace with real data
        val galleryPics = makeLongListOfTheSameItem(Constants.IMAGE_URL_EXAMPLE, 15).toMutableList()
        galleryPics.add(Constants.THUMBNAIL_URL_EXAMPLE)
        galleryPics.add(Constants.THUMBNAIL_URL_EXAMPLE)
        galleryPics.add(Constants.IMAGE_URL_EXAMPLE)
        _place.update {
            PlaceFull(
                id = 1,
                name = "Гора Эмина",
                rating = 5.0,
                pic = Constants.IMAGE_URL_EXAMPLE,
                excerpt = null,
                description = htmlExample,
                placeLocation = PlaceLocation(name = "Гора Эмина", lat = 38.579, lon = 68.782),
                pics = galleryPics,
            )
        }
    }
}

sealed interface UiEvent {
    data class ShowToast(val message: String) : UiEvent
}

val htmlExample =
    """
        <!DOCTYPE html>
        <html lang="tg">
        <head>
            <meta charset="UTF-8">
        </head>
        <body>
            <h2>Гиссарская крепость</h2>
            <p>⭐️ 4,8  работает каждый день, с 8:00 по 17:00</p>
            <h3>О месте</h3>
            <p>Город республиканского подчинения в западной части Таджикистана, в 20 километрах от столицы.</p>
            <p>Город славится историческими достопримечательностями, например, Гиссарской крепостью, которая считается одним из самых известных исторических сооружений в Центральной Азии.</p>
            <h3>Адрес</h3>
            <p>районы республиканского подчинения, город Гиссар с административной территорией, джамоат Хисор, село Гиссар</p>
            <h3>Контакты</h3>
            <p>+992 998 201 201</p>
        </body>
        </html>

    """.trimIndent()