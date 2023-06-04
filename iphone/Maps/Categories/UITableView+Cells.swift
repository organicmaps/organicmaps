extension UITableView {
  @objc func registerNib(cellClass: AnyClass) {
    register(UINib(cellClass), forCellReuseIdentifier: toString(cellClass))
  }

  @objc func registerClass(cellClass: AnyClass) {
    register(cellClass, forCellReuseIdentifier: toString(cellClass))
  }

  @objc func dequeueReusableCell(withCellClass cellClass: AnyClass) -> UITableViewCell? {
    return dequeueReusableCell(withIdentifier: toString(cellClass))
  }

  @objc func dequeueReusableCell(withCellClass cellClass: AnyClass, indexPath: IndexPath) -> UITableViewCell {
    return dequeueReusableCell(withIdentifier: toString(cellClass), for: indexPath)
  }

  func registerNib<Cell>(cell: Cell.Type) where Cell: UITableViewCell {
    register(UINib(cell), forCellReuseIdentifier: toString(cell))
  }

  func registerNibs<Cell>(_ cells: [Cell.Type]) where Cell: UITableViewCell {
    cells.forEach { registerNib(cell: $0) }
  }

  func register<Cell>(cell: Cell.Type) where Cell: UITableViewCell {
    register(cell, forCellReuseIdentifier: toString(cell))
  }

  func dequeueReusableCell<Cell>(cell: Cell.Type) -> Cell? where Cell: UITableViewCell {
    return dequeueReusableCell(withIdentifier: toString(cell)) as? Cell
  }

  func dequeueReusableCell<Cell>(cell: Cell.Type, indexPath: IndexPath) -> Cell where Cell: UITableViewCell {
    return dequeueReusableCell(withIdentifier: toString(cell), for: indexPath) as! Cell
  }

  func registerNibForHeaderFooterView<View>(_ view: View.Type) where View: UIView {
    register(UINib(view), forHeaderFooterViewReuseIdentifier: toString(view))
  }

  func dequeueReusableHeaderFooterView<View>(_ view: View.Type) -> View where View: UIView {
    return dequeueReusableHeaderFooterView(withIdentifier: toString(view)) as! View
  }
}
