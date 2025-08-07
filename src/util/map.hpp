#pragma once

#include <functional>
#include <vector>

namespace az
{
	template<typename ElemInT, typename ElemOutT>
	std::vector<ElemOutT> Map(const std::vector<ElemInT>& source, const std::function<ElemOutT(const ElemInT&)>& func);
}

#define MAP(ElemInT, ElemOutT, source_, func_) \
	[](const vector<ElemInT>& source, const function<ElemOutT(const ElemInT&)>& func) -> vector<ElemOutT> \
	{ \
		vector<ElemOutT> result; \
		transform(source.begin(), source.end(), back_inserter(result), func); \
		return result; \
	}(source_, func_)
