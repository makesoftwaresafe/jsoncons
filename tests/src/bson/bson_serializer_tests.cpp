// Copyright 2016 Daniel Parker
// Distributed under Boost license

#include <catch/catch.hpp>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/bson/bson.hpp>
#include <sstream>
#include <vector>
#include <utility>
#include <ctime>
#include <limits>

using namespace jsoncons;
using namespace jsoncons::bson;

TEST_CASE("serialize array to bson")
{
    std::vector<uint8_t> v;
    bson_bytes_serializer serializer(v);
    //serializer.begin_object(1);
    serializer.begin_array(3);
    serializer.bool_value(true);
    serializer.bool_value(false);
    serializer.null_value();
    serializer.end_array();
    //serializer.end_object();
    serializer.flush();

    try
    {
        json result = decode_bson<json>(v);
        std::cout << result << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }

} 

