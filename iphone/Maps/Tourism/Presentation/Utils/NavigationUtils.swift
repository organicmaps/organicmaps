extension UIViewController {
  func goToPlaceScreen(id: Int64) {
    let destinationVC = PlaceViewController(placeId: id)
    self.navigationController?.pushViewController(destinationVC, animated: false)
    self.tabBarController?.tabBar.isHidden = true
  }
}
