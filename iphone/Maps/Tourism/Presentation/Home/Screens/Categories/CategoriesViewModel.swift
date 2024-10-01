import Combine

class CategoriesViewModel: ObservableObject {
  private var cancellables = Set<AnyCancellable>()
  
  private let placesRepository: PlacesRepository
  
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
  
  init(placesRepository: PlacesRepository) {
    self.placesRepository = placesRepository
    
    if let firstCategory = categories.first {
      self.selectedCategory = firstCategory
    }
    
    observeCategoryPlaces()
  }
  
  func observeCategoryPlaces() {
    placesRepository.placesByCategoryResource.sink { completion in
      if case .failure(_) = completion {
        // nothing
      }
    } receiveValue: { places in
      self.places = places
    }
    .store(in: &cancellables)
    
    $selectedCategory.sink { category in
      if let id = category?.id.id {
        self.placesRepository.observePlacesByCategoryAndUpdate(categoryId: id)
      }
    }
    .store(in: &cancellables)
  }
  
  func toggleFavorite(for placeId: Int64, isFavorite: Bool) {
    placesRepository.setFavorite(placeId: placeId, isFavorite: isFavorite)
  }
}
