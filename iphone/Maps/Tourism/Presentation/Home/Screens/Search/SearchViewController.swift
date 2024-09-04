import SwiftUI

class SearchViewController: UIViewController {
  private var searchVM: SearchViewModel
  
  init(searchVM: SearchViewModel) {
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
    
    integrateSwiftUIScreen(SearchScreen(searchVM: searchVM))
  }
}

struct SearchScreen: View {
  @ObservedObject var searchVM: SearchViewModel
  @Environment(\.presentationMode) var presentationMode: Binding<PresentationMode>
  
  var body: some View {
    ScrollView {
      VStack(alignment: .leading) {
        VerticalSpace(height: 16)
        VStack {
          AppTopBar(
            title: L("search"),
            onBackClick: {
              presentationMode.wrappedValue.dismiss()
            }
          )
          
          AppSearchBar(
            query: $searchVM.query,
            onClearClicked: {
              searchVM.clearQuery()
            }
          )
        }
        .padding(16)
        
        VStack(spacing: 20) {
          
          LazyVStack(spacing: 16) {
            ForEach(searchVM.places) { place in
              PlacesItem(
                place: place,
                onPlaceClick: { clickedPlace in
                  // Handle place click
                  print("Place clicked: \(clickedPlace.name)")
                },
                onFavoriteChanged: { isFavorite in
                  searchVM.toggleFavorite(for: place.id, isFavorite: isFavorite)
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
