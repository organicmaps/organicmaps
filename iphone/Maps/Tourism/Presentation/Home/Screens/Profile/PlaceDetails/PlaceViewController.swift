import SwiftUI

class PlaceViewController: UIViewController {
  let placeId: Int64
  let onMapClick: () -> Void
  let onCreateRoute: (PlaceLocation) -> Void
  
  init(
    placeId: Int64,
    onMapClick: @escaping () -> Void,
    onCreateRoute: @escaping (PlaceLocation) -> Void
  ) {
    self.placeId = placeId
    self.onMapClick = onMapClick
    self.onCreateRoute = onCreateRoute
    
    super.init(
      nibName: nil,
      bundle: nil
    )
  }
  
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    let placesRepository = PlacesRepositoryImpl(
      placesService: PlacesServiceImpl(),
      placesPersistenceController: PlacesPersistenceController.shared,
      reviewsPersistenceController: ReviewsPersistenceController.shared,
      hashesPersistenceController: HashesPersistenceController.shared
    )
    let placeVM = PlaceViewModel(placesRepository: placesRepository, id: self.placeId)
    
    integrateSwiftUIScreen(PlaceScreen(
      placeVM: placeVM,
      id: placeId,
      showBottomBar: {
        self.tabBarController?.tabBar.isHidden = false
      },
      onMapClick: onMapClick,
      onCreateRoute: onCreateRoute
    ))
  }
}

struct PlaceScreen: View {
  @ObservedObject var placeVM: PlaceViewModel
  let reviewsVM: ReviewsViewModel
  let id: Int64
  let showBottomBar: () -> Void
  let onMapClick: () -> Void
  let onCreateRoute: (PlaceLocation) -> Void
  
  @State private var selectedTab = 0
  
  @Environment(\.presentationMode) var presentationMode: Binding<PresentationMode>
  
  init(
    placeVM: PlaceViewModel,
    id: Int64,
    showBottomBar: @escaping () -> Void,
    onMapClick: @escaping () -> Void,
    onCreateRoute: @escaping (PlaceLocation) -> Void
  ) {
    self.placeVM = placeVM
    self.id = id
    self.showBottomBar = showBottomBar
    self.onMapClick = onMapClick
    self.onCreateRoute = onCreateRoute
    
    self.reviewsVM = ReviewsViewModel(
        reviewsRepository: ReviewsRepositoryImpl(
          reviewsPersistenceController: ReviewsPersistenceController.shared,
          reviewsService: ReviewsServiceImpl(userPreferences: UserPreferences.shared)
        ),
        userPreferences: UserPreferences.shared,
        id: id
      )
  }
  
  var body: some View {
    if let place = placeVM.place {
      VStack {
        PlaceTopBar(
          title: place.name,
          picUrl: place.cover,
          onBackClick: {
            showBottomBar()
            presentationMode.wrappedValue.dismiss()
          },
          isFavorite: place.isFavorite,
          onFavoriteChanged: { isFavorite in
            placeVM.toggleFavorite(for: place.id, isFavorite: isFavorite)
          },
          onMapClick: onMapClick
        )
        
        VStack {
          PlaceTabsBar(selectedTab: $selectedTab)
            .padding()
          
          SwiftUI.TabView(selection: $selectedTab) {
            DescriptionScreen(
              description: place.description,
              onCreateRoute: {
                if let location = place.placeLocation {
                  onCreateRoute(location)
                }
              }
            )
            .tag(0)
            GalleryScreen(urls: place.pics)
              .tag(1)
            ReviewsScreen(
              reviewsVM: reviewsVM,
              placeId: place.id,
              rating: place.rating
            )
              .tag(2)
          }
          .tabViewStyle(PageTabViewStyle(indexDisplayMode: .never))
        }
      }
      .edgesIgnoringSafeArea(.all)
    }
  }
}
