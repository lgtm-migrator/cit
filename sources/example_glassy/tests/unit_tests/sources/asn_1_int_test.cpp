#include <doctest/doctest.h>
#include <outcome.hpp>

#include <asn_1_int.h>

#include <algorithm>
#include <functional>
#include <vector>

namespace glassy {
namespace test {

using namespace OUTCOME_V2_NAMESPACE;

struct Asn1IntTestData {
  std::string subcase_name{};
  std::function<Result<Asn1Int>()> create{};
  Long value{0};
};

#define CALL(X) []() { return Asn1Int::X; }

SCENARIO("Asn1Int creation and conversions") {
  const auto tests = std::vector<Asn1IntTestData>{
      {"default number", CALL(New()), 0},
      {"default number", CALL(New(0)), 0},
      {"default number", CALL(New(32)), 32},
  };

  std::for_each(std::begin(tests), std::end(tests),
                [](auto test) -> Result<void> {
                  SUBCASE(test.subcase_name.c_str()) {
                    OUTCOME_TRY(const auto number, test.create());
                    CHECK_EQ(number.ToLong().value(), test.value);
                  }

                  return success();
                });
}

SCENARIO("Asn1Int comparison") {
  SUBCASE("compare two Asn1Int values") {
    []() -> Result<void> {
      OUTCOME_TRY(const auto zero, Asn1Int::New(0));
      OUTCOME_TRY(const auto one, Asn1Int::New(1));
      SUBCASE("compare") {
        CHECK_EQ(Compare(zero, zero), 0);
        CHECK_EQ(Compare(one, one), 0);
        CHECK_EQ(Compare(zero, one), -1);
        CHECK_EQ(Compare(one, zero), 1);
      }
      SUBCASE("less") {
        CHECK(zero < one);
        CHECK_FALSE(one < zero);
        CHECK_FALSE(zero < zero);
        CHECK_FALSE(one < one);
      }
      SUBCASE("greater") {
        CHECK(one > zero);
        CHECK_FALSE(zero > one);
        CHECK_FALSE(zero > zero);
        CHECK_FALSE(one > one);
      }
      SUBCASE("equal") {
        CHECK(zero == zero);
        CHECK(one == one);
        CHECK_FALSE(zero == one);
        CHECK_FALSE(one == zero);
      }
      SUBCASE("not equal") {
        CHECK(zero != one);
        CHECK(one != zero);
        CHECK_FALSE(zero != zero);
        CHECK_FALSE(one != one);
      }
      return success();
    }()
                .value();
  }
}

}  // namespace test
}  // namespace glassy
