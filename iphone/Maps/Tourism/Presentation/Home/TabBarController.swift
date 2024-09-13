import UIKit
import SwiftUI
import Combine

class TabBarController: UITabBarController {
  
  override func viewDidAppear(_ animated: Bool) {
    if let theme = UserPreferences.shared.getTheme() {
      changeTheme(themeCode: theme.code)
    }
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    hidesBottomBarWhenPushed = true
    
    // creating tabs
    let homeTab = UITabBarItem(title: L("home"), image: UIImage(systemName: "house"), selectedImage: UIImage(systemName: "house.fill"))
    let categoriesTab = UITabBarItem(title: L("categories"), image: UIImage(systemName: "list.bullet.rectangle"), selectedImage: UIImage(systemName: "list.bullet.rectangle.fill"))
    let favoritesTab = UITabBarItem(title: L("favorites"), image: UIImage(systemName: "heart"), selectedImage: UIImage(systemName: "heart.fill"))
    let profileTab = UITabBarItem(title: L("tourism_profile"), image: UIImage(systemName: "person"), selectedImage: UIImage(systemName: "person.fill"))
    
    // creating navs
    let homeNav = UINavigationController()
    let categoriesNav = UINavigationController()
    let favoritesNav = UINavigationController()
    let profileNav = UINavigationController()
    
    // creating shared ViewModels
    let categoriesVM = CategoriesViewModel()
    let searchVM = SearchViewModel()
    let profileVM = ProfileViewModel(
      currencyRepository: CurrencyRepositoryImpl(
        currencyService: CurrencyServiceImpl(),
        currencyPersistenceController: CurrencyPersistenceController.shared
      ),
      profileRepository: ProfileRepositoryImpl (
        profileService: ProfileServiceImpl(userPreferences: UserPreferences.shared),
        personalDataPersistenceController: PersonalDataPersistenceController.shared
      ),
      authRepository: AuthRepositoryImpl(authService: AuthServiceImpl()),
      userPreferences: UserPreferences.shared
    )
    profileVM.onSignOutCompleted = {
      self.performSegue(withIdentifier: "TourismMain2Auth", sender: nil)
    }
    
    // navigation functions
    let goToCategoriesTab = { self.selectedIndex = 1 }
    
    // creating ViewControllers
    let homeVC = HomeViewController(
      categoriesVM: categoriesVM,
      searchVM: searchVM,
      goToCategoriesTab: goToCategoriesTab
    )
    let categoriesVC = CategoriesViewController(
      categoriesVM: categoriesVM,
      searchVM: searchVM
    )
    let favoritesVC = FavoritesViewController()
    let profileVC = ProfileViewController(profileVM: profileVM)
    
    // setting up navigation
    homeNav.viewControllers = [homeVC]
    categoriesNav.viewControllers = [categoriesVC]
    favoritesNav.viewControllers = [favoritesVC]
    profileNav.viewControllers = [profileVC]
    
    homeNav.tabBarItem = homeTab
    categoriesNav.tabBarItem = categoriesTab
    favoritesNav.tabBarItem = favoritesTab
    profileNav.tabBarItem = profileTab
    
    viewControllers = [homeNav, categoriesNav, favoritesNav, profileNav]
    PlacePersistenceControllerTesterBro.testAll()
  }
}


class PlacePersistenceControllerTesterBro {
  private static var cancellables = Set<AnyCancellable>()
  private static let persistenceController = PlacePersistenceController.shared
  private static let searchQuery = "place"
  
  static func testAll() {
    testSearchOperation()
    testPlacesByCatFetchOperation()
    testTopPlacesFetchOperation()
    testSinglePlaceFetchOperation()
    testFavoritePlacesFetchOperation()
    testCRUDOperations()
  }
  
  private static func testCRUDOperations() {
    print("Testing CRUD Operations...")
    
    // Example PlaceFull object
    let place = PlaceFull(
      id: 1,
      name: "Test Place",
      rating: 5,
      excerpt: "A great place",
      description: "Detailed description",
      placeLocation: nil,
      cover: Constants.imageUrlExample,
      pics: [Constants.imageUrlExample, Constants.imageUrlExample, Constants.anotherImageExample],
      reviews: nil,
      isFavorite: true
    )
    
    let place2 = PlaceFull(
      id: 2,
      name: "Test Place 2222",
      rating: 4.9,
      excerpt: "A great place",
      description: "Detailed description",
      placeLocation: nil,
      cover: Constants.imageUrlExample,
      pics: [Constants.imageUrlExample, Constants.imageUrlExample, Constants.anotherImageExample],
      reviews: nil,
      isFavorite: true
    )
    
    let place3 = PlaceFull(
      id: 3,
      name: "Test Place 3",
      rating: 5,
      excerpt: "A great place",
      description: "Detailed description",
      placeLocation: nil,
      cover: Constants.imageUrlExample,
      pics: [Constants.imageUrlExample, Constants.imageUrlExample, Constants.anotherImageExample],
      reviews: nil,
      isFavorite: false
    )
    
    var place4 = PlaceFull(
      id: 4,
      name: "Test Place 4",
      rating: 4,
      excerpt: "A great place",
      description: "Detailed description",
      placeLocation: nil,
      cover: Constants.imageUrlExample,
      pics: [Constants.imageUrlExample, Constants.imageUrlExample, Constants.anotherImageExample],
      reviews: nil,
      isFavorite: false
    )
    
    // Insert or update place
    DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
      self.persistenceController.putPlaces([place], categoryId: 1)
      print("Inserted/Updated places with ID: \(place.id)")
    }
    DispatchQueue.main.asyncAfter(deadline: .now() + 3) {
      self.persistenceController.putPlaces([place2, place3], categoryId: 2)
      print("Inserted/Updated places with ID: \(place2.id), \(place3.id)")
    }
    DispatchQueue.main.asyncAfter(deadline: .now() + 5) {
      self.persistenceController.putPlaces([place4], categoryId: 3)
      print("Inserted/Updated places with ID: \(place4.id)")
    }
    DispatchQueue.main.asyncAfter(deadline: .now() + 9) {
      place4.isFavorite = !place4.isFavorite
      self.persistenceController.putPlaces([place4], categoryId: 3)
      print("Inserted/Updated places with ID: \(place4.id)")
    }
    // Delete all
    DispatchQueue.main.asyncAfter(deadline: .now() + 0) {
      self.persistenceController.deleteAllPlaces()
      print("Deleted all places")
    }
    // Delete places by category (assuming `categoryId` is available)
    DispatchQueue.main.asyncAfter(deadline: .now() + 7) {
      self.persistenceController.deleteAllPlacesByCategory(categoryId: 3)
      print("Deleted places with category ID: 2")
    }
  }
  
  private static func testSearchOperation() {
    print("Testing Search Operation...")
    persistenceController.searchSubject
      .sink(receiveCompletion: { completion in
        if case .failure(let error) = completion {
          print("Search failed with error: \(error)")
        }
      }, receiveValue: { places in
        print("Search Results:")
        places.forEach { place in
          print("ID: \(place.id), Name: \(place.name ?? ""), Excerpt: \(place.excerpt ?? "No excerpt")")
        }
      })
      .store(in: &cancellables)
    
    persistenceController.observeSearch(query: searchQuery)
  }
  
  private static func testPlacesByCatFetchOperation() {
    print("Testing PlacesByCat Operation...")
    persistenceController.placesByCatSubject
      .sink(receiveCompletion: { completion in
        if case .failure(let error) = completion {
          print("PlacesByCat failed with error: \(error)")
        }
      }, receiveValue: { places in
        print("PlacesByCat Results:")
        places.forEach { place in
          print("ID: \(place.id), Name: \(place.name ?? ""), Excerpt: \(place.excerpt ?? "No excerpt")")
        }
      })
      .store(in: &cancellables)
    
    persistenceController.observePlacesByCategoryId(categoryId: 3)
  }
  
  private static func testTopPlacesFetchOperation() {
    print("Testing TopPlaces Operation...")
    persistenceController.topPlacesSubject
      .sink(receiveCompletion: { completion in
        if case .failure(let error) = completion {
          print("TopPlaces failed with error: \(error)")
        }
      }, receiveValue: { places in
        print("TopPlaces Results:")
        places.forEach { place in
          print("ID: \(place.id), Name: \(place.name ?? ""), Excerpt: \(place.excerpt ?? "No excerpt")")
        }
      })
      .store(in: &cancellables)
    
    persistenceController.observeTopPlacesByCategoryId(categoryId: 2)
  }
  
  private static func testSinglePlaceFetchOperation() {
    print("Testing SinglePlace Operation...")
    persistenceController.singlePlaceSubject
      .sink(receiveCompletion: { completion in
        if case .failure(let error) = completion {
          print("SinglePlace failed with error: \(error)")
        }
      }, receiveValue: { place in
        print("SinglePlace Results:")
        if let place = place {
          print("ID: \(place.id), Name: \(place.name ?? ""), Excerpt: \(place.excerpt ?? "No excerpt")")
        }
      })
      .store(in: &cancellables)
    
    persistenceController.observePlaceById(placeId: 1)
  }
  
  private static func testFavoritePlacesFetchOperation() {
    print("Testing FavoritePlaces Operation...")
    persistenceController.favoritePlacesSubject
      .sink(receiveCompletion: { completion in
        if case .failure(let error) = completion {
          print("FavoritePlaces failed with error: \(error)")
        }
      }, receiveValue: { places in
        print("FavoritePlaces Results:")
        places.forEach { place in
          print("ID: \(place.id), Name: \(place.name ?? ""), Excerpt: \(place.excerpt ?? "No excerpt")")
        }
      })
      .store(in: &cancellables)
    
    persistenceController.observeFavoritePlaces()
  }
}
