extension UITableView {
  typealias Updates = () -> Void
  typealias Completion = () -> Void

  @objc func update(_ updates: Updates) {
    performBatchUpdates(updates, completion: nil)
  }

  @objc func update(_ updates: Updates, completion: @escaping Completion) {
    performBatchUpdates(updates, completion: { _ in
      completion()
    })
  }

  @objc func refresh() {
    update {}
  }
}
