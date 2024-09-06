import Combine

class PlaceViewModel : ObservableObject {
  @Published var place: PlaceFull?
  
  init() {
    place = Constants.placeExample
  }
}
