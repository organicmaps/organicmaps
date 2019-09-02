#pragma once

#include <memory>

class Purchase;

namespace crown
{
bool NeedToShow(std::unique_ptr<Purchase> const & purchase);
}  // namespace crown
