#ifndef _json_fifo_h
#define _json_fifo_h
// This is a workaround/hack to allow nlohmann::json objects to have properties be output in First In/First Out order vs the
// alphabetized order which is the default behavior.  This hack is based on this issue filed on GitHub:
// https://github.com/nlohmann/json/issues/485#issuecomment-333652309
//
#include <nlohmann/json.hpp>
#include <fifo_map.hpp>


template<class K, class V, class dummy_compare, class A>
using my_workaround_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
using json = nlohmann::basic_json<my_workaround_fifo_map>;
#endif

