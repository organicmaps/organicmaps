import SwiftUI
import Combine

class HomeViewController: UIViewController {
  private var categoriesVM: CategoriesViewModel
  private var searchVM: SearchViewModel
  private var goToCategoriesTab: () -> Void
  
  init(categoriesVM: CategoriesViewModel, searchVM: SearchViewModel, goToCategoriesTab: @escaping () -> Void) {
    self.categoriesVM = categoriesVM
    self.searchVM = searchVM
    self.goToCategoriesTab = goToCategoriesTab
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
        homeVM: HomeViewModel(),
        categoriesVM: categoriesVM,
        goToCategoriesTab: goToCategoriesTab,
        goToSearchScreen: { query in
          let destinationVC = SearchViewController(searchVM: self.searchVM)
          self.navigationController?.pushViewController(destinationVC, animated: true)
        }
      )
    )
  }
}

struct HomeScreen: View {
  @ObservedObject var homeVM: HomeViewModel
  @ObservedObject var categoriesVM: CategoriesViewModel
  var goToCategoriesTab: () -> Void
  var goToSearchScreen: (String) -> Void
  
  @State var top30: SingleChoiceItem<Int>? = SingleChoiceItem(id: 1, label: L("top30"))
  
  var body: some View {
    ScrollView {
      VStack (alignment: .leading) {
        VerticalSpace(height: 16)
        VStack {
          AppTopBar(title: L("tjk"))
          
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
                
              },
              setFavoriteChanged: { place, isFavorite in
                
              }
            )
          }
          
          if let restaurants = homeVM.restaurants {
            HorizontalPlaces(
              title: L("restaurants"),
              items: restaurants,
              onPlaceClick: { place in
                
              },
              setFavoriteChanged: { place, isFavorite in
                
              }
            )
          }
        }
      }
      VerticalSpace(height: 32)
    }
  }
}

