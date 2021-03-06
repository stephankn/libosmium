#include "catch.hpp"

#include <sstream>

#include <osmium/osm/box.hpp>
#include <osmium/geom/relations.hpp>

TEST_CASE("Box") {

SECTION("instantiation") {
    osmium::Box b;
    REQUIRE(!b);
    REQUIRE(!b.bottom_left());
    REQUIRE(!b.top_right());
    REQUIRE_THROWS_AS(b.size(), osmium::invalid_location);
}

SECTION("instantiation_and_extend_with_undefined") {
    osmium::Box b;
    REQUIRE(!b);
    b.extend(osmium::Location());
    REQUIRE(!b.bottom_left());
    REQUIRE(!b.top_right());
}

SECTION("instantiation_and_extend") {
    osmium::Box b;
    b.extend(osmium::Location(1.2, 3.4));
    REQUIRE(!!b);
    REQUIRE(!!b.bottom_left());
    REQUIRE(!!b.top_right());
    b.extend(osmium::Location(3.4, 4.5));
    b.extend(osmium::Location(5.6, 7.8));
    REQUIRE(b.bottom_left() == osmium::Location(1.2, 3.4));
    REQUIRE(b.top_right() == osmium::Location(5.6, 7.8));

    // extend with undefined doesn't change anything
    b.extend(osmium::Location());
    REQUIRE(b.bottom_left() == osmium::Location(1.2, 3.4));
    REQUIRE(b.top_right() == osmium::Location(5.6, 7.8));
}

SECTION("output_defined") {
    osmium::Box b;
    b.extend(osmium::Location(1.2, 3.4));
    b.extend(osmium::Location(5.6, 7.8));
    std::stringstream out;
    out << b;
    REQUIRE(out.str() == "(1.2,3.4,5.6,7.8)");
    REQUIRE(19.36 == b.size());
}

SECTION("output_undefined") {
    osmium::Box b;
    std::stringstream out;
    out << b;
    REQUIRE(out.str() == "(undefined)");
}

SECTION("box_inside_box") {
    osmium::Box outer;
    outer.extend(osmium::Location(1, 1));
    outer.extend(osmium::Location(10, 10));

    osmium::Box inner;
    inner.extend(osmium::Location(2, 2));
    inner.extend(osmium::Location(4, 4));

    osmium::Box overlap;
    overlap.extend(osmium::Location(3, 3));
    overlap.extend(osmium::Location(5, 5));

    REQUIRE( osmium::geom::contains(inner, outer));
    REQUIRE(!osmium::geom::contains(outer, inner));

    REQUIRE(!osmium::geom::contains(overlap, inner));
    REQUIRE(!osmium::geom::contains(inner, overlap));
}

}

