import Combine

class PlaceViewModel : ObservableObject {
  private var cancellables = Set<AnyCancellable>()
  
  private let placesRepository: PlacesRepository
  
  @Published var place: PlaceFull?
  
  init(placesRepository: PlacesRepository, id: Int64) {
    self.placesRepository = placesRepository
    
    observePlace(id: id)
  }
  
  func observePlace(id: Int64) {
    placesRepository.placeResource
      .sink(receiveCompletion: { _ in }, receiveValue: { place in
        self.place = place
      })
      .store(in: &cancellables)
    
    placesRepository.observePlaceById(id)
  }
  
  func toggleFavorite(for placeId: Int64, isFavorite: Bool) {
    placesRepository.setFavorite(placeId: placeId, isFavorite: isFavorite)
  }
}
