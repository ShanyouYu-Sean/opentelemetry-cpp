// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stddef.h>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <cstring>

#include "opentelemetry/common/attribute_value.h"
#include "opentelemetry/common/key_value_iterable.h"
#include "opentelemetry/nostd/function_ref.h"
#include "opentelemetry/nostd/string_view.h"
#include "opentelemetry/nostd/variant.h"
#include "opentelemetry/sdk/common/attribute_utils.h"
#include "opentelemetry/version.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace sdk
{
namespace common
{

// FNV-1a 哈希常量
constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037U;
constexpr uint64_t FNV_64_PRIME = 1099511628211U;

// FNV-1a 哈希实现
inline size_t Fnv1aHash(const char* data, size_t length)
{
  uint64_t hash = FNV_OFFSET_BASIS;
  for (size_t i = 0; i < length; ++i)
  {
    hash ^= static_cast<unsigned char>(data[i]);
    hash *= FNV_64_PRIME;
  }
  return static_cast<size_t>(hash);
}

inline size_t Fnv1aHash(const std::string& str)
{
  return Fnv1aHash(str.data(), str.size());
}

// 允许接受任意类型的 Fnv1a 哈希函数
template <typename T>
inline size_t Fnv1aHash(const T& obj)
{
  std::ostringstream oss;
  oss << obj;
  return Fnv1aHash(oss.str());
}

void GetHash(size_t &seed, const std::string& arg)
{
  // FNV-1a 哈希
  seed ^= Fnv1aHash(arg) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <class T>
inline void GetHash(size_t &seed, const T &arg)
{
  // FNV-1a 哈希
  seed ^= Fnv1aHash(arg) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <class T>
inline void GetHash(size_t &seed, const std::vector<T> &arg)
{
  for (const auto& v : arg)
  {
    GetHash<T>(seed, v);
  }
}

// Specialization for const char*
// this creates an intermediate string.
template <>
inline void GetHash<const char *>(size_t &seed, const char *const &arg)
{
  seed ^= Fnv1aHash(arg, std::strlen(arg)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct GetHashForAttributeValueVisitor
{
  GetHashForAttributeValueVisitor(size_t &seed) : seed_(seed) {}
  template <class T>
  void operator()(T &v)
  {
    GetHash(seed_, v);
  }
  size_t &seed_;
};

// Calculate hash of keys and values of attribute map
#if defined(ENABLE_ATTRIBUTES_PROCESSOR)
inline size_t GetHashForAttributeMap(const OrderedAttributeMap &attribute_map)
#elif defined(ENABLE_GENERIC_ATTRIBUTES)
inline size_t GetHashForAttributeMap(const AttributeMap &attribute_map)
#else
inline size_t GetHashForAttributeMap(const StringAttributeMap &attribute_map)
#endif
{
  size_t seed = 0UL;
  for (auto &kv : attribute_map)
  {
    GetHash(seed, kv.first);
#if defined(ENABLE_GENERIC_ATTRIBUTES)
    nostd::visit(GetHashForAttributeValueVisitor(seed), kv.second);
#else
    GetHash(seed, kv.second);
#endif
  }
  return seed;
}

// Calculate hash of keys and values of KeyValueIterable, filtered using callback.
inline size_t GetHashForAttributeMap(
    const opentelemetry::common::KeyValueIterable &attributes,
    nostd::function_ref<bool(nostd::string_view)> is_key_present_callback)
{
  AttributeConverter converter;
  size_t seed = 0UL;
  attributes.ForEachKeyValue(
      [&](nostd::string_view key, opentelemetry::common::AttributeValue value) noexcept {
        if (!is_key_present_callback(key))
        {
          return true;
        }
        GetHash(seed, key);
        auto attr_val = nostd::visit(converter, value);
        nostd::visit(GetHashForAttributeValueVisitor(seed), attr_val);
        return true;
      });
  return seed;
}

template <class T>
inline size_t GetHash(T value)
{
  return Fnv1aHash(value);
}

}  // namespace common
}  // namespace sdk
OPENTELEMETRY_END_NAMESPACE
