import Foundation

func IPAD() -> Bool { return UI_USER_INTERFACE_IDIOM() == .pad }

func L(_ key: String) -> String { return NSLocalizedString(key, comment: "") }
