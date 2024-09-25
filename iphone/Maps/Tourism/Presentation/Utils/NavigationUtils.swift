extension UIViewController {
  func goToPlaceScreen(
    id: Int64,
    onMapClick: @escaping () -> Void,
    onCreateRoute: @escaping (PlaceLocation) -> Void
  ) {
    let destinationVC = PlaceViewController(
      placeId: id,
      onMapClick: onMapClick,
      onCreateRoute: onCreateRoute
    )
    self.navigationController?.pushViewController(destinationVC, animated: false)
    self.tabBarController?.tabBar.isHidden = true
  }
}
