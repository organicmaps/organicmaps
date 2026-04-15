extension UITableView {
  @objc func registerNib(cellClass: AnyClass) {
    register(UINib(cellClass), forCellReuseIdentifier: toString(cellClass))
  }

  @objc func registerClass(cellClass: AnyClass) {
    register(cellClass, forCellReuseIdentifier: toString(cellClass))
  }

  @objc func dequeueReusableCell(withCellClass cellClass: AnyClass) -> UITableViewCell? {
    dequeueReusableCell(withIdentifier: toString(cellClass))
  }

  @objc func dequeueReusableCell(withCellClass cellClass: AnyClass, indexPath: IndexPath) -> UITableViewCell {
    dequeueReusableCell(withIdentifier: toString(cellClass), for: indexPath)
  }

  func registerNib<Cell: UITableViewCell>(cell: Cell.Type) {
    register(UINib(cell), forCellReuseIdentifier: toString(cell))
  }

  func registerNibs<Cell: UITableViewCell>(_ cells: [Cell.Type]) {
    cells.forEach { registerNib(cell: $0) }
  }

  func register<Cell: UITableViewCell>(cell: Cell.Type) {
    register(cell, forCellReuseIdentifier: toString(cell))
  }

  func dequeueReusableCell<Cell: UITableViewCell>(cell: Cell.Type) -> Cell? {
    dequeueReusableCell(withIdentifier: toString(cell)) as? Cell
  }

  func dequeueReusableCell<Cell: UITableViewCell>(cell: Cell.Type, indexPath: IndexPath) -> Cell {
    dequeueReusableCell(withIdentifier: toString(cell), for: indexPath) as! Cell
  }

  func registerNibForHeaderFooterView<View: UIView>(_ view: View.Type) {
    register(UINib(view), forHeaderFooterViewReuseIdentifier: toString(view))
  }

  func dequeueReusableHeaderFooterView<View: UIView>(_ view: View.Type) -> View {
    dequeueReusableHeaderFooterView(withIdentifier: toString(view)) as! View
  }
}
