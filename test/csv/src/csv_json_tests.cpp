// Copyright 2013-2024 Daniel Parker
// Distributed under Boost license

#include <jsoncons/json.hpp>
#include <jsoncons_ext/csv/csv.hpp>
#include <catch/catch.hpp>

namespace csv = jsoncons::csv; 

TEST_CASE("test csv to json")
{
    SECTION("test 1")
    {
        std::string jtext = R"(
[
    {
        "text": "Chicago Reader", 
        "float": 1.0, 
        "datetime": "1971-01-01T04:14:00", 
        "boolean": true,
        "nested": {
          "time": "04:14:00",
          "nested": {
            "date": "1971-01-01",
            "integer": 40
          }
        }
    }, 
    {
        "text": "Chicago Sun-Times", 
        "float": 1.27, 
        "datetime": "1948-01-01T14:57:13", 
        "boolean": true,
        "nested": {
          "time": "14:57:13",
          "nested": {
            "date": "1948-01-01",
            "integer": 63
          }
        }
    }
]
        )";

        auto j = jsoncons::json::parse(jtext);
        std::cout << pretty_print(j) << "\n";

        std::string buf;
        csv::csv_string_encoder encoder(buf);
        j.dump(encoder);
        
        std::cout << buf << "\n";
    }
}


