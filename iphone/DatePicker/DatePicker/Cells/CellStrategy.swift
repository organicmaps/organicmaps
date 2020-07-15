import UIKit

enum PositionInRow {
  case outside
  case first
  case middle
  case last
  case single
}

enum PositionInRange {
  case inactive
  case outside
  case first
  case middle
  case last
  case single
}

final class CellStrategy {
  enum CellId: String {
    case empty = "empty"
    case regular = "regular"
    case inactive = "inactive"
    case selectedSingle = "selectedSingle"
    case selectedFirst = "selectedFirst"
    case selectedLast = "selectedLast"
    case rangeSingle = "rangeSingle"
    case rangeFirst = "rangeFirst"
    case rangeMiddle = "rangeMiddle"
    case rangeLast = "rangeLast"
  }

  weak var collectionView: UICollectionView?

  init(_ collectionView: UICollectionView) {
    self.collectionView = collectionView
    register(EmptyCell.self, cellId: .empty)
    register(Cell.self, cellId: .regular)
    register(InactiveCell.self, cellId: .inactive)
    register(SelectedSingleCell.self, cellId: .selectedSingle)
    register(SelectedFirstCell.self, cellId: .selectedFirst)
    register(SelectedLastCell.self, cellId: .selectedLast)
    register(RangeSingleCell.self, cellId: .rangeSingle)
    register(RangeFirstCell.self, cellId: .rangeFirst)
    register(RangeMiddleCell.self, cellId: .rangeMiddle)
    register(RangeLastCell.self, cellId: .rangeLast)
  }

  func cell(positionInRange: PositionInRange, positionInRow: PositionInRow, indexPath: IndexPath) -> Cell {
    let cellId: CellId
    switch (positionInRange, positionInRow) {
    case (_, .outside):
      cellId = .empty
    case (.inactive, _):
      cellId = .inactive
    case (.middle, .first):
      cellId = .rangeFirst
    case (.middle, .last):
      cellId = .rangeLast
    case (.middle, .single):
      cellId = .rangeSingle
    case (.single, _), (.first, .last), (.last, .first), (.first, .single), (.last, .single):
      cellId = .selectedSingle
    case (.outside, _):
      cellId = .regular
    case (.middle, _):
      cellId = .rangeMiddle
    case (.first, _):
      cellId = .selectedFirst
    case (.last, _):
      cellId = .selectedLast
    }

    return dequeueCell(cellId: cellId, indexPath: indexPath)
  }

  private func register(_ cellClass: AnyClass, cellId: CellId) {
    collectionView?.register(cellClass, forCellWithReuseIdentifier: cellId.rawValue)
  }

  private func dequeueCell(cellId: CellId, indexPath: IndexPath) -> Cell {
    collectionView!.dequeueReusableCell(withReuseIdentifier: cellId.rawValue, for: indexPath) as! Cell
  }
}
