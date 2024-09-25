import SwiftUI
import Combine

class HomeViewController: UIViewController {
  private var homeVM: HomeViewModel
  private var categoriesVM: CategoriesViewModel
  private var searchVM: SearchViewModel
  private var goToCategoriesTab: () -> Void
  private var goToMap: () -> Void
  private let onCreateRoute: (PlaceLocation) -> Void
  
  init(
    homeVM: HomeViewModel,
    categoriesVM: CategoriesViewModel,
    searchVM: SearchViewModel,
    goToCategoriesTab: @escaping () -> Void,
    goToMap: @escaping () -> Void,
    onCreateRoute: @escaping (PlaceLocation) -> Void
  ) {
    self.homeVM = homeVM
    self.categoriesVM = categoriesVM
    self.searchVM = searchVM
    self.goToCategoriesTab = goToCategoriesTab
    self.goToMap = goToMap
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
    
    integrateSwiftUIScreen(
      HomeScreen(
        homeVM: homeVM,
        categoriesVM: categoriesVM,
        goToCategoriesTab: goToCategoriesTab,
        goToSearchScreen: { query in
          self.searchVM.query = query
          let destinationVC = SearchViewController(
            searchVM: self.searchVM,
            goToMap: self.goToMap,
            onCreateRoute: self.onCreateRoute
          )
          self.navigationController?.pushViewController(destinationVC, animated: false)
        },
        goToPlaceScreen: { id in
          self.goToPlaceScreen(
            id: id,
            onMapClick: self.goToMap,
            onCreateRoute: self.onCreateRoute
          )
        },
        goToMap: goToMap
      )
    )
  }
}

struct HomeScreen: View {
  @ObservedObject var homeVM: HomeViewModel
  @ObservedObject var categoriesVM: CategoriesViewModel
  var goToCategoriesTab: () -> Void
  var goToSearchScreen: (String) -> Void
  var goToPlaceScreen: (Int64) -> Void
  var goToMap: () -> Void
  
  @State var top30: SingleChoiceItem<Int>? = SingleChoiceItem(id: 1, label: L("top30"))
  
  var body: some View {
    if(homeVM.downloadProgress == .loading) {
      VStack(spacing: 16) {
        ProgressView()
        Text(L("plz_wait_dowloading"))
      }
    } else if (homeVM.downloadProgress == .error) {
      VStack(spacing: 16) {
        Text(L("download_failed"))
        Text(homeVM.errorMessage)
      }
    } else if (homeVM.downloadProgress == .success) {
      ScrollView {
        VStack (alignment: .leading) {
          VerticalSpace(height: 16)
          VStack {
            AppTopBar(
              title: L("tjk"),
              actions: [
                TopBarActionData(
                  iconName: "map",
                  onClick: goToMap
                )
              ]
            )
            
            AppSearchBar(
              query: $homeVM.query,
              onSearchClicked: { query in
                goToSearchScreen(query)
              },
              onClearClicked: {
                homeVM.clearQuery()
              }
            )
          }
          .padding(16)
          
          VStack(spacing: 20) {
            ScrollView(.horizontal, showsIndicators: false) {
              HStack {
                HorizontalSpace(width: 16)
                SingleChoiceItemView(
                  item: top30!,
                  isSelected: true,
                  onClick: {
                    // nothing, just static
                  },
                  selectedColor: Color.selected,
                  unselectedColor: Color.background
                )
                
                HorizontalSingleChoice(
                  items: categoriesVM.categories,
                  selected: $categoriesVM.selectedCategory,
                  onSelectedChanged: { item in
                    categoriesVM.setSelectedCategory(item)
                    goToCategoriesTab()
                  }
                )
              }
            }
            
            if let sights = homeVM.sights {
              HorizontalPlaces(
                title: L("sights"),
                items: sights,
                onPlaceClick: { place in
                  goToPlaceScreen(place.id)
                },
                setFavoriteChanged: { place, isFavorite in
                  homeVM.toggleFavorite(for: place.id, isFavorite: isFavorite)
                }
              )
            }
            
            if let restaurants = homeVM.restaurants {
              HorizontalPlaces(
                title: L("restaurants"),
                items: restaurants,
                onPlaceClick: { place in
                  goToPlaceScreen(place.id)
                },
                setFavoriteChanged: { place, isFavorite in
                  homeVM.toggleFavorite(for: place.id, isFavorite: isFavorite)
                }
              )
            }
          }
        }
        VerticalSpace(height: 32)
      }
    }
  }
}

