#include "route.hpp"

namespace routing
{

Route::Route(vector<m2::PointD> const & points) : m_poly(points)
{
}

} // namespace routing
