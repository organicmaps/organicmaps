#include "user_marks_provider.hpp"

namespace df
{
UserPointMark::UserPointMark(kml::MarkId id) : m_id(id) {}

UserLineMark::UserLineMark(kml::TrackId id) : m_id(id) {}

}  // namespace df
