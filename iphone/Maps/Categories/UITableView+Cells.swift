extension UITableView {
  func register(cellClass: AnyClass) {
    register(UINib(cellClass), forCellReuseIdentifier: toString(cellClass))
  }

  func dequeueReusableCell(withCellClass cellClass: AnyClass) -> UITableViewCell? {
    return dequeueReusableCell(withIdentifier: toString(cellClass))
  }

  func dequeueReusableCell(withCellClass cellClass: AnyClass, indexPath: IndexPath) -> UITableViewCell {
    return dequeueReusableCell(withIdentifier: toString(cellClass), for: indexPath)
  }
}
