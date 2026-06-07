#include "Platform/PoolString.h"

namespace Awful
{

	std::vector<std::string> PoolString::sPool = { "" };

	PoolString PoolString::Invalid;
}