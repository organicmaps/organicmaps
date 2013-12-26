#pragma once

#include "uniform_value.hpp"

#include "../std/vector.hpp"
#include "../std/string.hpp"
#include "../std/function.hpp"

class UniformValuesStorage
{
public:
  void SetIntValue(const string & name, int32_t v);
  void SetIntValue(const string & name, int32_t v1, int32_t v2);
  void SetIntValue(const string & name, int32_t v1, int32_t v2, int32_t v3);
  void SetIntValue(const string & name, int32_t v1, int32_t v2, int32_t v3, int32_t v4);

  void SetFloatValue(const string & name, float v);
  void SetFloatValue(const string & name, float v1, float v2);
  void SetFloatValue(const string & name, float v1, float v2, float v3);
  void SetFloatValue(const string & name, float v1, float v2, float v3, float v4);

  void SetMatrix4x4Value(const string & name, float * matrixValue);

  typedef function<void (UniformValue const & )> enum_uniforms_fn;
  void ForeachValue(enum_uniforms_fn action) const;

  bool operator< (const UniformValuesStorage & other) const;

private:
  UniformValue * findByName(const string & name);

private:
  typedef vector<UniformValue> uniforms_t;
  uniforms_t m_uniforms;
};
