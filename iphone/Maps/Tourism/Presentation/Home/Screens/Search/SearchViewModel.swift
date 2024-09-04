import Combine

class SearchViewModel: ObservableObject {
  @Published var query = ""
  
  func clearQuery() { query = "" }
  
  @Published var places: [PlaceShort] = []
  
  init() {
    // TODO: put real data
    places = [
      PlaceShort(id: 1, name: "sight 1", cover: Constants.imageUrlExample, rating: 4.5, excerpt: "yep, just a placeyep, just a placeyep, just a placeyep, just a placeyep, just a place", isFavorite: false),
      PlaceShort(id: 2, name: "sight 2", cover: Constants.imageUrlExample, rating: 4.0, excerpt: "yep, just a place", isFavorite: true)
    ]
  }
  
  func toggleFavorite(for placeId: Int64, isFavorite: Bool) {
    if let index = places.firstIndex(where: { $0.id == placeId }) {
      places[index].isFavorite = isFavorite
    }
  }
}
