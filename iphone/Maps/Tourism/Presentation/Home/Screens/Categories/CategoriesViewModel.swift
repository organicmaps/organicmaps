import Combine

class CategoriesViewModel: ObservableObject {
  @Published var query = ""
  
  func clearQuery() { query = "" }
  
  var categories = [
    SingleChoiceItem(id: PlaceCategory.sights, label: L("sights")),
    SingleChoiceItem(id: PlaceCategory.restaurants, label: L("restaurants")),
    SingleChoiceItem(id: PlaceCategory.hotels, label: L("hotels"))
  ]
  
  @Published var selectedCategory: SingleChoiceItem<PlaceCategory>?
  
  func setSelectedCategory(_ item: SingleChoiceItem<PlaceCategory>?) {
    selectedCategory = item
  }
  
  @Published var places: [PlaceShort] = []
  
  init() {
    if let firstCategory = categories.first {
      self.selectedCategory = firstCategory
    }
    
    // TODO: put real data
    places = [
      PlaceShort(id: 1, name: "sight 1", cover: Constants.imageUrlExample, rating: 4.5, excerpt: "yep, just a placeyep, just a placeyep, just a placeyep, just a placeyep, just a placejust a placeyep, just a placejust a placeyep, just a placejust a placeyep, just a placejust a placeyep, just a place", isFavorite: false),
      PlaceShort(id: 2, name: "sight 2", cover: Constants.imageUrlExample, rating: 4.0, excerpt: "yep, just a place", isFavorite: true)
    ]
  }
  
  func toggleFavorite(for placeId: Int64, isFavorite: Bool) {
    if let index = places.firstIndex(where: { $0.id == placeId }) {
      places[index].isFavorite = isFavorite
    }
  }
}
