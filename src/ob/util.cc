#include "ob/util.hh"

#include <string>
#include <fstream>

namespace OB::Util
{

bool file_exists(std::string const& str)
{
  return static_cast<bool>(std::ifstream(str));
}

} // namespace OB::Util
