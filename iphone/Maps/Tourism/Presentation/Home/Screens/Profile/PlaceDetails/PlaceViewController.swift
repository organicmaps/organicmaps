import UIKit
import SwiftUI

class PlaceViewController: UIViewController {
  override func viewDidLoad() {
    super.viewDidLoad()
    
    let placeVM = PlaceViewModel()
    integrateSwiftUIScreen(PlaceScreen(placeVM: placeVM))
  }
}

struct PlaceScreen: View {
  @ObservedObject var placeVM: PlaceViewModel
  
  @State private var selectedTab = 0
  
  var body: some View {
    TabView() {
        Text("Tab 1")
            .tabItem {
                Label("First Tab", systemImage: "house")
            }
        Text("Tab 2")
            .tabItem {
                Label("Second Tab", systemImage: "person")
            }
        // Add more tabs as needed
    }
      
  }
}


struct FirstScreen: View {
  var body: some View {
    VStack {
      Text("First Screen")
      NavigationLink("Go to Detail", destination: DetailScreen())
    }
    .navigationTitle("First Tab")
  }
}

struct SecondScreen: View {
  var body: some View {
    VStack {
      Text("Second Screen")
      NavigationLink("Go to Detail", destination: DetailScreen())
    }
    .navigationTitle("Second Tab")
  }
}

struct ThirdScreen: View {
  var body: some View {
    VStack {
      Text("Third Screen")
      NavigationLink("Go to Detail", destination: DetailScreen())
    }
    .navigationTitle("Third Tab")
  }
}

struct DetailScreen: View {
  var body: some View {
    Text("Detail Screen")
      .navigationTitle("Detail")
  }
}
