import UIKit
import SwiftUI

class TabBarController: UITabBarController {
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    let firstTab = UITabBarItem(title: L("home"), image: UIImage(systemName: "house"), selectedImage: UIImage(systemName: "house.fill"))
    let secondTab = UITabBarItem(title: L("profile"), image: UIImage(systemName: "person"), selectedImage: UIImage(systemName: "person.fill"))
    
    let homeNav = UINavigationController()
    let profileNav = UINavigationController()
    
    let homeVC = HomeViewController()
    let profileVC = ProfileViewController()
    
    homeNav.viewControllers = [homeVC]
    profileNav.viewControllers = [profileVC]
    
    homeNav.tabBarItem = firstTab
    profileNav.tabBarItem = secondTab
    
    viewControllers = [homeNav, profileNav]
  }
}

