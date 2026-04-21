#pragma once

namespace vec_util
{
template <typename A, typename F> auto map(const A &v, F &&f);
template <typename A, typename F> auto filter(const A &v, F &&pred);
template <typename A, typename F> void for_each(const A &v, F &&f);
} // namespace vec_util
