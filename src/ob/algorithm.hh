#ifndef OB_ALGORITHM_HH
#define OB_ALGORITHM_HH

#include <cstddef>

#include <functional>

namespace OB::Algorithm
{

template<class Op>
void for_each(std::size_t const cn, Op const& op)
{
  if (cn == 0) return;
  for (std::size_t i = 0; i < cn; ++i)
  {
    op(i);
  }
}

template<class Cn, class Op>
void for_each(Cn const& cn, Op const& op)
{
  if (cn.empty()) return;
  for (std::size_t i = 0; i < cn.size(); ++i)
  {
    op(cn[i]);
  }
}

template<class Cn, class Op1, class Op2>
void for_each(Cn const& cn, Op1 const& op_n, Op2 const& op_l)
{
  if (cn.empty()) return;
  for (std::size_t i = 0; i < cn.size() - 1; ++i)
  {
    op_n(cn[i]);
  }
  op_l(cn.back());
}

template<class Op1, class Op2>
void for_each(std::size_t const& n, Op1 const& op_n, Op2 const& op_l)
{
  if (n == 0) return;
  for (std::size_t i = 0; i < n - 1; ++i)
  {
    op_n(i);
  }
  op_l(n - 1);
}

} // namespace OB::Algorithm

#endif // OB_ALGORITHM_HH
