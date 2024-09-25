import SwiftUI

class CategoriesViewController: UIViewController {
  private var categoriesVM: CategoriesViewModel
  private var searchVM: SearchViewModel
  private var goToMap: () -> Void
  private let onCreateRoute: (PlaceLocation) -> Void
  
  init(
    categoriesVM: CategoriesViewModel,
    searchVM: SearchViewModel,
    goToMap: @escaping () -> Void,
    onCreateRoute: @escaping (PlaceLocation) -> Void
  ) {
    self.categoriesVM = categoriesVM
    self.searchVM = searchVM
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
      CategoriesScreen(
        categoriesVM: categoriesVM,
        goToSearchScreen: { query in
          self.searchVM.query = query
          let destinationVC = SearchViewController(
            searchVM: self.searchVM,
            goToMap: self.goToMap,
            onCreateRoute: self.onCreateRoute
          )
          self.navigationController?.pushViewController(destinationVC, animated: true)
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

struct CategoriesScreen: View {
  @ObservedObject var categoriesVM: CategoriesViewModel
  var goToSearchScreen: (String) -> Void
  var goToPlaceScreen: (Int64) -> Void
  var goToMap: () -> Void
  
  var body: some View {
    ScrollView {
      VStack(alignment: .leading) {
        VerticalSpace(height: 16)
        VStack {
          AppTopBar(
            title: L("categories"),
            actions: [
              TopBarActionData(
                iconName: "map",
                onClick: goToMap
              )
            ]
          )
          
          AppSearchBar(
            query: $categoriesVM.query,
            onSearchClicked: { query in
              goToSearchScreen(query)
            },
            onClearClicked: {
              categoriesVM.clearQuery()
            }
          )
        }
        .padding(16)
        
        VStack(spacing: 20) {
          HorizontalSingleChoice(
            items: categoriesVM.categories,
            selected: $categoriesVM.selectedCategory,
            onSelectedChanged: { item in
              categoriesVM.setSelectedCategory(item)
            }
          )
          
          LazyVStack(spacing: 16) {
            ForEach(categoriesVM.places) { place in
              PlacesItem(
                place: place,
                onPlaceClick: { place in
                  goToPlaceScreen(place.id)
                },
                onFavoriteChanged: { isFavorite in
                  categoriesVM.toggleFavorite(for: place.id, isFavorite: isFavorite)
                }
              )
            }
          }
          .padding(.horizontal, 16)
        }
        VerticalSpace(height: 32)
      }
    }
  }
}
