import Combine

class FavoritesViewModel: ObservableObject {
  private var cancellables = Set<AnyCancellable>()
  
  private let placesRepository: PlacesRepository
  
  @Published var query = ""
  
  func clearQuery() { query = "" }
  
  @Published var places: [PlaceShort] = []
  
  init(placesRepository: PlacesRepository) {
    self.placesRepository = placesRepository
    
    self.placesRepository.observeFavorites(query: query)
    observeFavorites()
  }
  
  func observeFavorites() {
    placesRepository.favoritesResource.sink { completion in
      if case let .failure(error) = completion {
        // nothing
      }
    } receiveValue: { places in
      self.places = places
    }
    .store(in: &cancellables)
    
    $query.sink { q in
      self.placesRepository.observeFavorites(query: q)
    }
    .store(in: &cancellables)
  }
  
  func toggleFavorite(for placeId: Int64, isFavorite: Bool) {
    placesRepository.setFavorite(placeId: placeId, isFavorite: isFavorite)
  }
}
