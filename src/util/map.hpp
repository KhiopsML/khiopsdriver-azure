#pragma once

#include <functional>
#include <vector>

namespace az
{
	template<typename ElemInT, typename ElemOutT>
	std::vector<ElemOutT> Map(const std::vector<ElemInT>& source, const std::function<ElemOutT(const ElemInT&)>& func);
}
