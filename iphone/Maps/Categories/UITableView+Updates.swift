extension UITableView {
  typealias Updates = () -> Void

  func update(_ updates: Updates) {
    beginUpdates()
    updates()
    endUpdates()
  }

  func refresh() {
    update {}
  }
}
