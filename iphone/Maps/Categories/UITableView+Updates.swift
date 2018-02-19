extension UITableView {
  typealias Updates = () -> Void
  typealias Completion = () -> Void

  @objc func update(_ updates: Updates) {
    if #available(iOS 11.0, *) {
      performBatchUpdates(updates, completion: nil)
    } else {
      beginUpdates()
      updates()
      endUpdates()
    }
  }

  @objc func update(_ updates: Updates, completion: @escaping Completion) {
    if #available(iOS 11.0, *) {
      performBatchUpdates(updates, completion: { _ in
        completion()
      })
    } else {
      CATransaction.begin()
      beginUpdates()
      CATransaction.setCompletionBlock(completion)
      updates()
      endUpdates()
      CATransaction.commit()
    }
  }

  @objc func refresh() {
    update {}
  }
}
