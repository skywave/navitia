#include "../indexes.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_indexes
#include <boost/test/unit_test.hpp>
#include <string>
#include "fixtures.h"



BOOST_AUTO_TEST_CASE(test_order){
    MyConfig f;
    auto city_ordrerd_by_name = order(f.cities, &City::name);
    BOOST_CHECK_EQUAL(city_ordrerd_by_name.begin()->name, "Barcelona");
    BOOST_CHECK_EQUAL((--(city_ordrerd_by_name.end()))->name, "Wien");
    int count = 0;
    BOOST_FOREACH(auto city, city_ordrerd_by_name){
        count++;
    }
    BOOST_CHECK_EQUAL(count,  f.cities.size());
}


BOOST_AUTO_TEST_CASE(test_filter){
    MyConfig f;
    auto city_of_france = filter(f.cities, &City::country, std::string("France"));
    int count = 0;
    BOOST_FOREACH(auto city, city_of_france){
        BOOST_CHECK_EQUAL(city.country, "France");
        count++;
    }
    BOOST_CHECK_EQUAL(count, 2);
}



BOOST_AUTO_TEST_CASE(test_join){
    MyConfig f;
    auto join = make_join(f.stops,
                          f.line_stops,
                          attribute_equals(&Stop::id, &LineStops::stop_id));
    int count = 0;
    BOOST_FOREACH(auto stop, join){
        BOOST_CHECK_EQUAL(join_get<LineStops>(stop).stop_id, join_get<Stop>(stop).id);
        count++;
    }
    BOOST_CHECK_GT(count, 0);
        
}


BOOST_AUTO_TEST_CASE(test_join_filtered){
    MyConfig f;
    auto join = make_join(f.stops,
                          filter(f.line_stops, &LineStops::line_name, std::string("Orient Express")),
                          attribute_equals(&Stop::id, &LineStops::stop_id));
    int count = 0;
    BOOST_FOREACH(auto stop, join){
        BOOST_CHECK_EQUAL(join_get<LineStops>(stop).line_name, "Orient Express");
        BOOST_CHECK_EQUAL(join_get<LineStops>(stop).stop_id, join_get<Stop>(stop).id);
        count++;
    }
    BOOST_CHECK_GT(count, 0);

}


BOOST_AUTO_TEST_CASE(test_join_filtered_alternate){
    MyConfig f;
    auto join = make_join(filter(f.line_stops, &LineStops::line_name, std::string("Orient Express")),
                          f.stops,
                          attribute_equals(&LineStops::stop_id, &Stop::id));
    
    int count = 0;
    BOOST_FOREACH(auto stop, join){
        BOOST_CHECK(join_get<LineStops>(stop).line_name == "Orient Express");
        BOOST_CHECK(join_get<LineStops>(stop).stop_id == join_get<Stop>(stop).id);
        count++;
    }
    BOOST_CHECK_GT(count, 0);
        
}



BOOST_AUTO_TEST_CASE(test_join_filtered_and_ordered){
    MyConfig f;
    auto join = make_join(filter(f.line_stops, &LineStops::line_name, std::string("Orient Express")),
                          order(f.stops, &Stop::name),
                          attribute_equals(&LineStops::stop_id, &Stop::id));
    int count = 0;
    BOOST_FOREACH(auto stop, join){
        BOOST_CHECK(join_get<LineStops>(stop).line_name == "Orient Express");
        BOOST_CHECK(join_get<LineStops>(stop).stop_id == join_get<Stop>(stop).id);
        count++;
    }
    BOOST_CHECK_GT(count, 0);

}


BOOST_AUTO_TEST_CASE(test_double_join){
    MyConfig f;
    auto join6 = make_join(filter(f.cities, &City::country, std::string("France")), f.stops, attribute_equals(&City::name, &Stop::city));
    auto join5 = make_join( f.line_stops,
                            join6,
                            attribute_equals(&LineStops::stop_id, &Stop::id));
    int count = 0;
    BOOST_FOREACH(auto stop, join5){
        BOOST_CHECK_EQUAL(join_get<LineStops>(stop).stop_id, join_get<Stop>(stop).id);
        BOOST_CHECK(!join_get<Stop>(stop).name.empty());
        BOOST_CHECK_EQUAL(join_get<City>(stop).country, "France");
        BOOST_CHECK_EQUAL(join_get<City>(stop).name, join_get<Stop>(stop).city);
        count++;
    }
    BOOST_CHECK_GT(count, 0);

}


BOOST_AUTO_TEST_CASE(test_filter_index){
    MyConfig f;
    
    Index<Stop> paris_stops_idx = make_index(filter(f.stops, &Stop::city, std::string("Paris")));
    int count = 0;
    BOOST_FOREACH(auto stop, paris_stops_idx){
        count++;
        BOOST_CHECK_EQUAL(stop.city, "Paris");
    }
    BOOST_CHECK_EQUAL(count, 2);
}



BOOST_AUTO_TEST_CASE(test_sorter_index){
    MyConfig f;
    
    Index<Stop> paris_stops_idx = make_index(order(f.stops, &Stop::name));
    BOOST_CHECK_EQUAL(paris_stops_idx.begin()->name, "Centre du monde");
    BOOST_CHECK_EQUAL((--(paris_stops_idx.end()))->name, "Westbahnhof");
    int count = 0;
    BOOST_FOREACH(auto stop, paris_stops_idx){
        count++;
    }
    BOOST_CHECK_EQUAL(count, f.stops.size());
}


BOOST_AUTO_TEST_CASE(test_StringIndex){
    MyConfig f;

    StringIndex<Stop> stops_str_idx = make_string_index(f.stops, &Stop::name);
    int count = 0;
    BOOST_FOREACH(auto stop, stops_str_idx) {
        count++;
    }
    BOOST_CHECK_EQUAL(stops_str_idx["Sants"].city, "Barcelona");
    BOOST_CHECK_THROW(stops_str_idx["foobar"], std::out_of_range);
    BOOST_CHECK_EQUAL(count, f.stops.size());
}

BOOST_AUTO_TEST_CASE(test_joinIndex){
    MyConfig f;
    auto join6 = make_join(filter(f.cities, &City::country, std::string("France")), f.stops, attribute_equals(&City::name, &Stop::city));
    auto join_idx = make_join_index(join6);
    BOOST_FOREACH(auto stop, join_idx){
        BOOST_CHECK_EQUAL(join_get2<City>(stop).country, "France");
        BOOST_CHECK_EQUAL(join_get2<City>(stop).name, join_get2<Stop>(stop).city);
        BOOST_CHECK(!join_get2<City>(stop).name.empty());
    }
}