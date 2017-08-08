extension UITableView {
  typealias Updates = () -> Void
  typealias Completion = () -> Void

  func update(_ updates: Updates) {
    beginUpdates()
    updates()
    endUpdates()
  }

  func update(_ updates: Updates, completion: @escaping Completion) {
    CATransaction.begin()
    beginUpdates()
    CATransaction.setCompletionBlock(completion)
    updates()
    endUpdates()
    CATransaction.commit()
  }

  func refresh() {
    update {}
  }
}
