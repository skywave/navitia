/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
This file is part of Navitia,
    the software to build cool stuff with public transport.
 
Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!
  
LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
   
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.
   
You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
  
Stay tuned using
twitter @navitia 
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_ed
#include <boost/test/unit_test.hpp>
#include "time_tables/departure_boards.h"
#include "ed/build_helper.h"
#include "type/type.h"

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )


using namespace navitia::timetables;

boost::gregorian::date date(std::string str) {
    return boost::gregorian::from_undelimited_string(str);
}

BOOST_AUTO_TEST_CASE(test1) {
    ed::builder b("20120614");
    b.vj("A")("stop1", 36000, 36100)("stop2", 36150,362000);
    b.vj("B")("stop1", 36000, 36100)("stop2", 36150,36200)("stop3", 36250,36300);
    b.data->pt_data->index();
    b.data->build_raptor();

    boost::gregorian::date begin = boost::gregorian::date_from_iso_string("20120613");
    boost::gregorian::date end = boost::gregorian::date_from_iso_string("20120630");

    b.data->meta->production_date = boost::gregorian::date_period(begin, end);

    pbnavitia::Response resp = departure_board("stop_point.uri=stop2", {}, {}, "20120615T094500", 86400, 0, std::numeric_limits<int>::max(), 1, 10, 0, *(b.data), false);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 2);
    pbnavitia::StopSchedule stop_schedule = resp.stop_schedules(0);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(),0);
    BOOST_CHECK_EQUAL(stop_schedule.response_status(), pbnavitia::ResponseStatus::terminus);
    stop_schedule = resp.stop_schedules(1);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(),1);

    resp = departure_board("stop_point.uri=stop2", {}, {}, "20120615T094500", 800, 0, std::numeric_limits<int>::max(), 1, 10, 0, *(b.data), false);

    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 2);
    stop_schedule = resp.stop_schedules(0);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(),0);
    BOOST_CHECK_EQUAL(stop_schedule.response_status(), pbnavitia::ResponseStatus::terminus);
    stop_schedule = resp.stop_schedules(1);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(),0);
    BOOST_CHECK_EQUAL(stop_schedule.response_status(), pbnavitia::ResponseStatus::no_departure_this_day);

    resp = departure_board("stop_point.uri=stop2", {}, {}, "20120701T094500", 86400, 0, std::numeric_limits<int>::max(), 1, 10, 0, *(b.data), false);
    BOOST_REQUIRE_EQUAL(resp.error().id(), pbnavitia::Error::date_out_of_bounds);
}


struct calendar_fixture {
    ed::builder b;
    boost::gregorian::date beg;
    boost::gregorian::date end_of_year;
    calendar_fixture() : b("20120614") {
        //2 vj during the week
        b.vj("network:R", "line:A", "1", "", true, "week")("stop1", 10 * 3600, 10 * 3600 + 10 * 60)("stop2", 12 * 3600, 12 * 3600 + 10 * 60);
        b.vj("network:R", "line:A", "101", "", true, "week_bis")("stop1", 11 * 3600, 11 * 3600 + 10 * 60)("stop2", 14 * 3600, 14 * 3600 + 10 * 60);
        //NOTE: we give a first random validity pattern because the builder try to factorize them

        //only one on the week end
        b.vj("network:R", "line:A", "10101", "", true, "weekend")("stop1", 20 * 3600, 20 * 3600 + 10 * 60)("stop2", 21 * 3600, 21 * 3600 + 10 * 60);

        // and one everytime
        b.vj("network:R", "line:A", "1100101", "", true, "all")("stop1", 15 * 3600, 15 * 3600 + 10 * 60)("stop2", 16 * 3600, 16 * 3600 + 10 * 60);

        // and wednesday that will not be matched to any cal
        b.vj("network:R", "line:A", "110010011", "", true, "wednesday")("stop1", 17 * 3600, 17 * 3600 + 10 * 60)("stop2", 18 * 3600, 18 * 3600 + 10 * 60);

        b.data->build_uri();
        beg = b.data->meta->production_date.begin();
        end_of_year = beg + boost::gregorian::years(1) + boost::gregorian::days(1);

        navitia::type::VehicleJourney* vj_week = b.data->pt_data->vehicle_journeys_map["week"];
        vj_week->validity_pattern->add(beg, end_of_year, std::bitset<7>{"1111100"});
        navitia::type::VehicleJourney* vj_week_bis = b.data->pt_data->vehicle_journeys_map["week_bis"];
        vj_week_bis->validity_pattern->add(beg, end_of_year, std::bitset<7>{"1111100"});
        navitia::type::VehicleJourney* vj_weekend = b.data->pt_data->vehicle_journeys_map["weekend"];
        vj_weekend->validity_pattern->add(beg, end_of_year, std::bitset<7>{"0000011"});
        navitia::type::VehicleJourney* vj_all = b.data->pt_data->vehicle_journeys_map["all"];
        vj_all->validity_pattern->add(beg, end_of_year, std::bitset<7>{"1111111"});
        navitia::type::VehicleJourney* vj_wednesday = b.data->pt_data->vehicle_journeys_map["wednesday"];
        vj_wednesday->validity_pattern->add(beg, end_of_year, std::bitset<7>{"0010000"});

        //we now add 2 similar calendars
        auto week_cal = new navitia::type::Calendar(b.data->meta->production_date.begin());
        week_cal->uri = "week_cal";
        week_cal->active_periods.push_back({beg, end_of_year});
        week_cal->week_pattern = std::bitset<7>{"1111100"};
        b.data->pt_data->calendars.push_back(week_cal);

        auto weekend_cal = new navitia::type::Calendar(b.data->meta->production_date.begin());
        weekend_cal->uri = "weekend_cal";
        weekend_cal->active_periods.push_back({beg, end_of_year});
        weekend_cal->week_pattern = std::bitset<7>{"0000011"};
        b.data->pt_data->calendars.push_back(weekend_cal);

        auto not_associated_cal = new navitia::type::Calendar(b.data->meta->production_date.begin());
        not_associated_cal->uri = "not_associated_cal";
        not_associated_cal->active_periods.push_back({beg, end_of_year});
        not_associated_cal->week_pattern = std::bitset<7>{"0010000"};
        b.data->pt_data->calendars.push_back(not_associated_cal); //not associated to the line

        //both calendars are associated to the line
        b.lines["line:A"]->calendar_list.push_back(week_cal);
        b.lines["line:A"]->calendar_list.push_back(weekend_cal);

        b.data->build_uri();
        b.data->pt_data->index();
        b.data->build_raptor();
        b.data->geo_ref->init();
        b.data->complete();

        //we chack that each vj is associated with the right calendar
        //NOTE: this is better checked in the UT for associated cal
        BOOST_REQUIRE_EQUAL(vj_week->associated_calendars.size(), 1);
        BOOST_REQUIRE(vj_week_bis->associated_calendars["week_cal"]);
        BOOST_REQUIRE_EQUAL(vj_week_bis->associated_calendars.size(), 1);
        BOOST_REQUIRE(vj_week_bis->associated_calendars["week_cal"]);
        BOOST_REQUIRE_EQUAL(vj_weekend->associated_calendars.size(), 1);
        BOOST_REQUIRE(vj_weekend->associated_calendars["weekend_cal"]);
        BOOST_REQUIRE_EQUAL(vj_all->associated_calendars.size(), 2);
        BOOST_REQUIRE(vj_all->associated_calendars["week_cal"]);
        BOOST_REQUIRE(vj_all->associated_calendars["weekend_cal"]);
        BOOST_REQUIRE(vj_wednesday->associated_calendars.empty());
    }
};

/*
 * unknown calendar in request => error
 */
BOOST_FIXTURE_TEST_CASE(test_no_weekend, calendar_fixture) {

    //when asked on non existent calendar, we get an error
    boost::optional<const std::string> calendar_id{"bob_the_calendar"};

    pbnavitia::Response resp = departure_board("stop_point.uri=stop1", calendar_id, {}, "20120615T080000", 86400, 0, std::numeric_limits<int>::max(), 1, 10, 0, *(b.data), false);

    BOOST_REQUIRE(resp.has_error());
    BOOST_REQUIRE(! resp.error().message().empty());
}

/*
 * For this test we want to get the schedule for the week end
 * we thus will get the 'week end' vj + the 'all' vj
 */
BOOST_FIXTURE_TEST_CASE(test_calendar_weekend, calendar_fixture) {
    boost::optional<const std::string> calendar_id{"weekend_cal"};

    pbnavitia::Response resp = departure_board("stop_point.uri=stop1", calendar_id, {}, "20120615T080000", 86400, 0, std::numeric_limits<int>::max(), 1, 10, 0, *(b.data), false);

    BOOST_REQUIRE(! resp.has_error());
    BOOST_CHECK_EQUAL(resp.stop_schedules_size(), 1);
    pbnavitia::StopSchedule stop_schedule = resp.stop_schedules(0);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(), 2);
    auto stop_date_time = stop_schedule.date_times(0);
    BOOST_CHECK_EQUAL(stop_date_time.date_time(), "T151000");
    stop_date_time = stop_schedule.date_times(1);
    BOOST_CHECK_EQUAL(stop_date_time.date_time(), "T201000");
    //the vj 'wednesday' is never matched
}

/*
 * For this test we want to get the schedule for the week
 * we thus will get the 2 'week' vj + the 'all' vj
 */
BOOST_FIXTURE_TEST_CASE(test_calendar_week, calendar_fixture) {

    boost::optional<const std::string> calendar_id{"week_cal"};

    pbnavitia::Response resp = departure_board("stop_point.uri=stop1", calendar_id, {}, "20120615T080000", 86400, 0, std::numeric_limits<int>::max(), 1, 10, 0, *(b.data), false);

    BOOST_REQUIRE(! resp.has_error());
    BOOST_CHECK_EQUAL(resp.stop_schedules_size(), 1);
    pbnavitia::StopSchedule stop_schedule = resp.stop_schedules(0);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(), 3);
    auto stop_date_time = stop_schedule.date_times(0);
    BOOST_CHECK_EQUAL(stop_date_time.date_time(), "T101000");
    stop_date_time = stop_schedule.date_times(1);
    BOOST_CHECK_EQUAL(stop_date_time.date_time(), "T111000");
    stop_date_time = stop_schedule.date_times(2);
    BOOST_CHECK_EQUAL(stop_date_time.date_time(), "T151000");
    //the vj 'wednesday' is never matched
}

/*
 * when asked with a calendar not associated with the line, we got an empty schedule
 */
BOOST_FIXTURE_TEST_CASE(test_not_associated_cal, calendar_fixture) {

    boost::optional<const std::string> calendar_id{"not_associated_cal"};

    pbnavitia::Response resp = departure_board("stop_point.uri=stop1", calendar_id, {}, "20120615T080000", 86400, 0, std::numeric_limits<int>::max(), 1, 10, 0, *(b.data), false);

    BOOST_REQUIRE(! resp.has_error());
    BOOST_CHECK_EQUAL(resp.stop_schedules_size(), 1);
    pbnavitia::StopSchedule stop_schedule = resp.stop_schedules(0);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(), 0);
}

BOOST_FIXTURE_TEST_CASE(test_calendar_with_exception, calendar_fixture) {
    //we add a new calendar that nearly match a vj
    auto nearly_cal = new navitia::type::Calendar(b.data->meta->production_date.begin());
    nearly_cal->uri = "nearly_cal";
    nearly_cal->active_periods.push_back({beg, end_of_year});
    nearly_cal->week_pattern = std::bitset<7>{"1111100"};
    //we add 2 exceptions (2 add), one by week
    navitia::type::ExceptionDate exception_date;
    exception_date.date = date("20120618");
    exception_date.type = navitia::type::ExceptionDate::ExceptionType::add;
    nearly_cal->exceptions.push_back(exception_date);
    exception_date.date = date("20120619");
    exception_date.type = navitia::type::ExceptionDate::ExceptionType::add;
    nearly_cal->exceptions.push_back(exception_date);


    b.data->pt_data->calendars.push_back(nearly_cal);
    b.lines["line:A"]->calendar_list.push_back(nearly_cal);

    //call all the init again
    b.data->build_uri();
    b.data->pt_data->index();
    b.data->build_raptor();

    b.data->complete();

    boost::optional<const std::string> calendar_id{"nearly_cal"};

    pbnavitia::Response resp = departure_board("stop_point.uri=stop1", calendar_id, {}, "20120615T080000", 86400, 0, std::numeric_limits<int>::max(), 1, 10, 0, *(b.data), false);

    //it should match only the 'all' vj
    BOOST_REQUIRE(! resp.has_error());
    BOOST_CHECK_EQUAL(resp.stop_schedules_size(), 1);
    pbnavitia::StopSchedule stop_schedule = resp.stop_schedules(0);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(), 3);
    auto stop_date_time = stop_schedule.date_times(0);
    BOOST_CHECK_EQUAL(stop_date_time.date_time(), "T101000");

    auto properties = stop_date_time.properties();
    BOOST_REQUIRE_EQUAL(properties.exceptions_size(), 2);
    auto exception = properties.exceptions(0);
    BOOST_REQUIRE_EQUAL(exception.uri(), "exception:020120618");
    BOOST_REQUIRE_EQUAL(exception.date(), "20120618");
    BOOST_REQUIRE_EQUAL(exception.type(), pbnavitia::ExceptionType::Add);

    exception = properties.exceptions(1);
    BOOST_REQUIRE_EQUAL(exception.uri(), "exception:020120619");
    BOOST_REQUIRE_EQUAL(exception.date(), "20120619");
    BOOST_REQUIRE_EQUAL(exception.type(), pbnavitia::ExceptionType::Add);

}
