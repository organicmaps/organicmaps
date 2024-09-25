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
    
    // navigation functions
    let goToCategoriesTab = { self.selectedIndex = 1 }
    let goToMap = {
      self.dismiss(animated: true)
    }
    let goToAuth = {
      self.performSegue(withIdentifier: "TourismMain2Auth", sender: nil)
    }
    let goToMapAndCreateRoute: (PlaceLocation) -> Void = { location in
      UserPreferences.shared.setLocation(value: location)
      self.dismiss(animated: true)
    }
    
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
    
    // creating repositories and shared ViewModels
    let placesRepository = PlacesRepositoryImpl(
      placesService: PlacesServiceImpl(),
      placesPersistenceController: PlacesPersistenceController.shared,
      reviewsPersistenceController: ReviewsPersistenceController.shared,
      hashesPersistenceController: HashesPersistenceController.shared
    )
    let currencyRepository = CurrencyRepositoryImpl(
      currencyService: CurrencyServiceImpl(),
      currencyPersistenceController: CurrencyPersistenceController.shared
    )
    let profileRepository = ProfileRepositoryImpl (
      profileService: ProfileServiceImpl(userPreferences: UserPreferences.shared),
      personalDataPersistenceController: PersonalDataPersistenceController.shared
    )
    let authRepository = AuthRepositoryImpl(authService: AuthServiceImpl())
    
    // monitoring network for sync
    let dataSyncer = DataSyncer(
      reviewsRepository: ReviewsRepositoryImpl(
        reviewsPersistenceController: ReviewsPersistenceController.shared,
        reviewsService: ReviewsServiceImpl(userPreferences: UserPreferences.shared)
      ),
      placesRepository: placesRepository
    )
    dataSyncer.startMonitoring()
    
    // creating shared viewModels()
    let homeVM = HomeViewModel(placesRepository: placesRepository)
    let categoriesVM = CategoriesViewModel(placesRepository: placesRepository)
    let favoritesVM = FavoritesViewModel(placesRepository: placesRepository)
    let searchVM = SearchViewModel(placesRepository: placesRepository)
    let profileVM = ProfileViewModel(
      currencyRepository: currencyRepository,
      profileRepository: profileRepository,
      authRepository: authRepository,
      userPreferences: UserPreferences.shared
    )
    profileVM.onSignOutCompleted = goToAuth
    
    // creating ViewControllers
    let homeVC = HomeViewController(
      homeVM: homeVM,
      categoriesVM: categoriesVM,
      searchVM: searchVM,
      goToCategoriesTab: goToCategoriesTab,
      goToMap: goToMap,
      onCreateRoute: goToMapAndCreateRoute
    )
    let categoriesVC = CategoriesViewController(
      categoriesVM: categoriesVM,
      searchVM: searchVM,
      goToMap: goToMap,
      onCreateRoute: goToMapAndCreateRoute
    )
    let favoritesVC = FavoritesViewController(
      favoritesVM: favoritesVM,
      goToMap: goToMap,
      onCreateRoute: goToMapAndCreateRoute
    )
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
  }
}
