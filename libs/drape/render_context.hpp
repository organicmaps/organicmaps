#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <typeindex>
#include <vector>

namespace dp
{
// State shared by the threads of one map, never by independent map views.
class RenderContext
{
public:
  static std::shared_ptr<RenderContext> Current() { return s_current ? s_current : Primary(); }

  template <typename T>
  static T & Get()
  {
    static thread_local RenderContext * cachedContext = nullptr;
    static thread_local size_t cachedGeneration = 0;
    static thread_local T * cachedValue = nullptr;
    auto const & context = s_current ? s_current : Primary();
    if (cachedContext == context.get() && cachedGeneration == context->m_generation && cachedValue)
      return *cachedValue;
    std::lock_guard lock(context->m_mutex);
    auto & value = context->m_values[std::type_index(typeid(T))];
    if (!value)
    {
      auto instance = std::shared_ptr<T>(new T(), [](T * p) { delete p; });
      value = instance;
      context->m_order.push_back(std::type_index(typeid(T)));
    }
    cachedContext = context.get();
    cachedGeneration = context->m_generation;
    cachedValue = static_cast<T *>(value.get());
    return *cachedValue;
  }

  // Bind this context and stop its rendering/task threads before releasing their state.
  void Clear()
  {
    m_generation = ++s_generation;
    while (!m_order.empty())
    {
      auto key = m_order.back();
      m_order.pop_back();
      m_values.erase(key);
      m_generation = ++s_generation;
    }
  }

  class Scope
  {
  public:
    explicit Scope(std::shared_ptr<RenderContext> context) : m_previous(std::move(s_current))
    {
      s_current = std::move(context);
    }
    ~Scope() { s_current = std::move(m_previous); }
    Scope(Scope const &) = delete;
    Scope & operator=(Scope const &) = delete;

  private:
    std::shared_ptr<RenderContext> m_previous;
  };

private:
  static std::shared_ptr<RenderContext> const & Primary()
  {
    static auto primary = std::make_shared<RenderContext>();
    return primary;
  }
  static thread_local std::shared_ptr<RenderContext> s_current;
  static std::atomic<size_t> s_generation;
  size_t m_generation = ++s_generation;
  std::recursive_mutex m_mutex;
  std::map<std::type_index, std::shared_ptr<void>> m_values;
  std::vector<std::type_index> m_order;
};
}  // namespace dp
