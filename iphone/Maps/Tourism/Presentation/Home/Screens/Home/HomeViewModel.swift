import Combine

class HomeViewModel : ObservableObject {
  @Published var query = ""
  
  func clearQuery() { query = "" }
  
  @Published var sights: [PlaceShort]? = nil
  @Published var restaurants: [PlaceShort]? = nil
  
  init() {
    // TODO: put real data
    sights = [
      PlaceShort(id: 1, name: "sight 1", cover: Constants.imageUrlExample, rating: 4.5, excerpt: "yep, just a placeyep, just a placeyep, just a placeyep, just a placeyep, just a place", isFavorite: false),
      PlaceShort(id: 2, name: "sight 2", cover: Constants.imageUrlExample, rating: 4.0, excerpt: "yep, just a place", isFavorite: true)
    ]
    
    restaurants = [
      PlaceShort(id: 1, name: "restaurant 1", cover: Constants.imageUrlExample, rating: 4.5, excerpt: "yep, just a placeyep, just a placeyep, just a placeyep, just a placeyep, just a place", isFavorite: false),
      PlaceShort(id: 2, name: "restaurant 2", cover: Constants.imageUrlExample, rating: 4.0, excerpt: "yep, just a place", isFavorite: true)
    ]
  }
}
