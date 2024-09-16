import Combine

class HomeViewModel : ObservableObject {
  private let placesRepository: PlacesRepository
  
  @Published var downloadProgress = DownloadProgress.idle
  @Published var errorMessage = ""
  
  @Published var query = ""
  
  func clearQuery() { query = "" }
  
  @Published var sights: [PlaceShort]? = nil
  @Published var restaurants: [PlaceShort]? = nil
  
  private var cancellables = Set<AnyCancellable>()
  
  init(placesRepository: PlacesRepository) {
    self.placesRepository = placesRepository
    
    downloadAllDataIfDidnt()
    observeDownloadProgress()
    observeTopSightsAndUpdate()
    observeTopRestaurantsAndUpdate()
  }
  
  func observeDownloadProgress() {
    placesRepository.downloadProgress.sink { completion in
      if case let .failure(error) = completion {
        Task { await MainActor.run {
            self.downloadProgress = .error
        }}
        self.errorMessage = error.errorDescription
      }
    } receiveValue: { progress in
      Task { await MainActor.run {
        self.downloadProgress = progress
      }}
    }
    .store(in: &cancellables)
  }
  
  func downloadAllDataIfDidnt() {
    Task {
      try await placesRepository.downloadAllData()
    }
  }
  
  func observeTopSightsAndUpdate() {
    placesRepository.topSightsResource.sink { _ in } receiveValue: { places in
      self.sights = places
    }
    .store(in: &cancellables)
    
    placesRepository.observeTopSightsAndUpdate()
  }
  
  func observeTopRestaurantsAndUpdate() {
    placesRepository.topRestaurantsResource.sink { _ in } receiveValue: { places in
      self.restaurants = places
    }
    .store(in: &cancellables)
    
    placesRepository.observeTopRestaurantsAndUpdate()
  }
  
  func toggleFavorite(for placeId: Int64, isFavorite: Bool) {
    placesRepository.setFavorite(placeId: placeId, isFavorite: isFavorite)
  }
}
