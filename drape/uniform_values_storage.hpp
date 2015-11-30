#pragma once

#include "drape/uniform_value.hpp"

#include "std/vector.hpp"
#include "std/string.hpp"
#include "std/function.hpp"

namespace dp
{

class UniformValuesStorage
{
public:
  void SetIntValue(string const & name, int32_t v);
  void SetIntValue(string const & name, int32_t v1, int32_t v2);
  void SetIntValue(string const & name, int32_t v1, int32_t v2, int32_t v3);
  void SetIntValue(string const & name, int32_t v1, int32_t v2, int32_t v3, int32_t v4);

  void SetFloatValue(string const & name, float v);
  void SetFloatValue(string const & name, float v1, float v2);
  void SetFloatValue(string const & name, float v1, float v2, float v3);
  void SetFloatValue(string const & name, float v1, float v2, float v3, float v4);

  void SetMatrix4x4Value(string const & name, float const * matrixValue);

  typedef function<void (UniformValue const & )> enum_uniforms_fn;
  void ForeachValue(enum_uniforms_fn action) const;

  bool operator< (UniformValuesStorage const & other) const;

private:
  UniformValue * findByName(string const & name);

private:
  typedef vector<UniformValue> uniforms_t;
  uniforms_t m_uniforms;
};

} // namespace dp
