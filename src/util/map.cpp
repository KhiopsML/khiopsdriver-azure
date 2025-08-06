#include "map.hpp"
#include <algorithm>
#include <iterator>

using namespace std;

namespace az
{
	template<typename ElemInT, typename ElemOutT>
	vector<ElemOutT> Map(const vector<ElemInT>& source, const function<ElemOutT(const ElemInT&)>& func)
	{
		vector<ElemOutT> result;
		transform(source.begin(), source.end(), back_inserter(result), func);
		return result;
	}
}
