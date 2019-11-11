#import <Foundation/Foundation.h>

#include "3party/opening_hours/opening_hours.hpp"
#include "editor/opening_hours_ui.hpp"
#include "base/assert.hpp"

NSDateComponents * dateComponentsFromTime(osmoh::Time const & time);
NSDate * dateFromTime(osmoh::Time const & time);
NSString * stringFromTime(osmoh::Time const & time);

NSString * stringFromOpeningDays(editor::ui::OpeningDays const & openingDays);

BOOL isEveryDay(editor::ui::TimeTable const & timeTable);
