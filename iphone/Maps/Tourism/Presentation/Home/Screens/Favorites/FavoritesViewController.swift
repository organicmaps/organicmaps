import SwiftUI

class FavoritesViewController: UIViewController {
  private var favoritesVM: FavoritesViewModel
  init(favoritesVM: FavoritesViewModel) {
    self.favoritesVM = favoritesVM
    
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
      FavoritesScreen(
        favoritesVM: favoritesVM,
        goToPlaceScreen: { id in
          self.goToPlaceScreen(id: id)
        }
      )
    )
  }
}

struct FavoritesScreen: View {
  @ObservedObject var favoritesVM: FavoritesViewModel
  var goToPlaceScreen: (Int64) -> Void
  
  var body: some View {
    ScrollView {
      VStack(alignment: .leading) {
        VerticalSpace(height: 16)
        VStack {
          AppTopBar(title: L("favorites"))
          
          AppSearchBar(
            query: $favoritesVM.query,
            onSearchClicked: { query in
              // nothing, actually, it will be real time
            },
            onClearClicked: {
              favoritesVM.clearQuery()
            }
          )
        }
        .padding(16)
        
        VStack(spacing: 20) {
          LazyVStack(spacing: 16) {
            ForEach(favoritesVM.places) { place in
              PlacesItem(
                place: place,
                onPlaceClick: { place in
                  goToPlaceScreen(place.id)
                },
                onFavoriteChanged: { isFavorite in
                  favoritesVM.toggleFavorite(for: place.id, isFavorite: isFavorite)
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
