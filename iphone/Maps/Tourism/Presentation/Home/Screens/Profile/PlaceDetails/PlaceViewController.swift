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
    
    let placeVM = PlaceViewModel()
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
  let id: Int64
  let showBottomBar: () -> Void
  
  @State private var selectedTab = 0
  
  @Environment(\.presentationMode) var presentationMode: Binding<PresentationMode>
  
  var body: some View {
    if let place = placeVM.place {
      VStack {
        PlaceTopBar(
          title: "place",
          picUrl: Constants.imageUrlExample,
          onBackClick: {
            showBottomBar()
            presentationMode.wrappedValue.dismiss()
          },
          isFavorite: false,
          onFavoriteChanged: { isFavorite in
            // TODO: Cmon
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
            ReviewsScreen(placeId: place.id, rating: place.rating)
              .tag(2)
          }
          .tabViewStyle(PageTabViewStyle(indexDisplayMode: .never))
        }
      }
      .edgesIgnoringSafeArea(.all)
    }
  }
}
