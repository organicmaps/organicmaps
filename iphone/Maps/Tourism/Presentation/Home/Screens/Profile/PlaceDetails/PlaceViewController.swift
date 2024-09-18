import SwiftUI

class PlaceViewController: UIViewController {
  let placeId: Int64
  
  init(placeId: Int64) {
    self.placeId = placeId
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
      }
    ))
  }
}

struct PlaceScreen: View {
  @ObservedObject var placeVM: PlaceViewModel
  let reviewsVM: ReviewsViewModel
  let id: Int64
  let showBottomBar: () -> Void
  
  @State private var selectedTab = 0
  
  @Environment(\.presentationMode) var presentationMode: Binding<PresentationMode>
  
  init(placeVM: PlaceViewModel, id: Int64, showBottomBar: @escaping () -> Void) {
    self.placeVM = placeVM
    self.id = id
    self.showBottomBar = showBottomBar
    
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
          onMapClick: {
            // TODO: Cmon
          }
        )
        
        VStack {
          PlaceTabsBar(selectedTab: $selectedTab)
            .padding()
          
          SwiftUI.TabView(selection: $selectedTab) {
            DescriptionScreen(
              description: place.description,
              onCreateRoute: {
                // TODO: cmon
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
