extension UITableView {
  @objc func register(cellClass: AnyClass) {
    register(UINib(cellClass), forCellReuseIdentifier: toString(cellClass))
  }

  @objc func dequeueReusableCell(withCellClass cellClass: AnyClass) -> UITableViewCell? {
    return dequeueReusableCell(withIdentifier: toString(cellClass))
  }

  @objc func dequeueReusableCell(withCellClass cellClass: AnyClass, indexPath: IndexPath) -> UITableViewCell {
    return dequeueReusableCell(withIdentifier: toString(cellClass), for: indexPath)
  }
}
