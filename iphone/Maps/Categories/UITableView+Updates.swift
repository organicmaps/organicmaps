extension UITableView {
  typealias Updates = () -> Void
  typealias Completion = () -> Void

  @objc func update(_ updates: Updates) {
    beginUpdates()
    updates()
    endUpdates()
  }

  @objc func update(_ updates: Updates, completion: @escaping Completion) {
    CATransaction.begin()
    beginUpdates()
    CATransaction.setCompletionBlock(completion)
    updates()
    endUpdates()
    CATransaction.commit()
  }

  @objc func refresh() {
    update {}
  }
}
