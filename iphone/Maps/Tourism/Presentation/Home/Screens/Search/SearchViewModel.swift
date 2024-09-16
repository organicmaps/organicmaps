import Combine

class SearchViewModel: ObservableObject {
  private var cancellables = Set<AnyCancellable>()
  
  private let placesRepository: PlacesRepository
  
  @Published var query = ""
  
  func clearQuery() { query = "" }
  
  @Published var places: [PlaceShort] = []
  
  init(placesRepository: PlacesRepository) {
    self.placesRepository = placesRepository
    
    observeSearch()
  }
  
  func observeSearch() {
    placesRepository.searchResource.sink { completion in
      if case let .failure(error) = completion {
        // nothing
      }
    } receiveValue: { places in
      self.places = places
    }
    .store(in: &cancellables)
    
    $query.sink { q in
      self.placesRepository.observeSearch(query: q)
    }
    .store(in: &cancellables)
  }
  
  func toggleFavorite(for placeId: Int64, isFavorite: Bool) {
    placesRepository.setFavorite(placeId: placeId, isFavorite: isFavorite)
  }
}
