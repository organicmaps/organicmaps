import SwiftUI

class CategoriesViewController: UIViewController {
  private var categoriesVM: CategoriesViewModel
  private var searchVM: SearchViewModel
  
  init(categoriesVM: CategoriesViewModel, searchVM: SearchViewModel) {
    self.categoriesVM = categoriesVM
    self.searchVM = searchVM
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
            let destinationVC = SearchViewController(searchVM: self.searchVM)
            self.navigationController?.pushViewController(destinationVC, animated: true)
        },
        goToPlaceScreen: { id in
          self.goToPlaceScreen(id: id)
        }
      )
    )
  }
}

struct CategoriesScreen: View {
  @ObservedObject var categoriesVM: CategoriesViewModel
  var goToSearchScreen: (String) -> Void
  var goToPlaceScreen: (Int64) -> Void
  
  var body: some View {
    ScrollView {
      VStack(alignment: .leading) {
        VerticalSpace(height: 16)
        VStack {
          AppTopBar(title: L("categories"))
          
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
