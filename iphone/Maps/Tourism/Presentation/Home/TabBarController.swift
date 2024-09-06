import UIKit
import SwiftUI

class TabBarController: UITabBarController {
  
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
    let searchViewModel = SearchViewModel()
    
    // navigation functions
    let goToCategoriesTab = { self.selectedIndex = 1 }
    
    // creating ViewControllers
    let homeVC = HomeViewController(
      categoriesVM: categoriesVM,
      searchVM: searchViewModel,
      goToCategoriesTab: goToCategoriesTab
    )
    let categoriesVC = CategoriesViewController(
      categoriesVM: categoriesVM,
      searchVM: searchViewModel
    )
    let favoritesVC = FavoritesViewController()
    let profileVC = ProfileViewController()
    
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

