package app.tourism.domain.models.categories

data class Category(val value: String?, val label: String)

enum class PlaceCategory(val id: Long) {
    Sights(1), // called attractions in the server
    Restaurants(2),
    Hotels(3) // called accommodations in the server
}