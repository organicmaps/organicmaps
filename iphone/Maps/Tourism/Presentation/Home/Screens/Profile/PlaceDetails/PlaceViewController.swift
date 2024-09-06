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
    VStack {
      PlaceTopBar(
        title: "place",
        picUrl: Constants.imageUrlExample,
        onBackClick: {
          presentationMode.wrappedValue.dismiss()
          showBottomBar()
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
        
        SwiftUI.TabView(selection: $selectedTab) {
          DescriptionScreen().tag(0)
          GalleryScreen().tag(1)
          ReviewsScreen().tag(2)
        }
        .tabViewStyle(PageTabViewStyle(indexDisplayMode: .never))
      }.padding(16)
    }
    .edgesIgnoringSafeArea(.all)
  }
}
