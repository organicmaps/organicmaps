#include "search/displayed_categories.hpp"

namespace
{
vector<string> const kDisplayedCategories = {
    "food", "hotel", "tourism",       "wifi",     "transport", "fuel",   "parking", "shop",
    "atm",  "bank",  "entertainment", "hospital", "pharmacy",  "police", "toilet",  "post"};
}  // namespace

namespace search
{
vector<string> const & GetDisplayedCategories() { return kDisplayedCategories; }
}  // namespace search
