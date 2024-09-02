import UIKit
import SwiftUI

class TabBarController: UITabBarController {
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    let homeTab = UITabBarItem(title: L("home"), image: UIImage(systemName: "house"), selectedImage: UIImage(systemName: "house.fill"))
    let profileTab = UITabBarItem(title: L("tourism_profile"), image: UIImage(systemName: "person"), selectedImage: UIImage(systemName: "person.fill"))
    
    let homeNav = UINavigationController()
    let profileNav = UINavigationController()
    
    let homeVC = HomeViewController()
    let profileVC = ProfileViewController()
    
    homeNav.viewControllers = [homeVC]
    profileNav.viewControllers = [profileVC]
    
    homeNav.tabBarItem = homeTab
    profileNav.tabBarItem = profileTab
    
    viewControllers = [homeNav, profileNav]
  }
}

