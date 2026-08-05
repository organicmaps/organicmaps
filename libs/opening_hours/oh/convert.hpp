#pragma once

// Bidirectional converters between the vendored `opening_hours` AST (port of
// opening-hours-rs, in oh/parser.hpp) and the `osmoh` AST used across
// Organic Maps (routing serdes, the editor, transit). The port drives parsing
// and evaluation; the osmoh AST is kept as the app-facing data model so that
// GetRule()/operator<< and the mwm serialization format stay unchanged.

#include "opening_hours/oh/parser.hpp"      // opening_hours AST
#include "opening_hours/opening_hours.hpp"  // osmoh AST

namespace osmoh
{
// port expression -> osmoh rule sequence (used to populate GetRule()).
TRuleSequences ToOsmoh(oh::OpeningHoursExpression const & expr);

// osmoh rule sequence -> port expression (used to evaluate a rule set that was
// built programmatically, e.g. deserialized from an mwm or from the editor).
oh::OpeningHoursExpression ToPort(TRuleSequences const & rules);
}  // namespace osmoh
