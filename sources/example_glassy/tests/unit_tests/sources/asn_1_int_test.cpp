// SPDX-FileCopyrightText: © 2022 Melg Eight <public.melg8@gmail.com>
//
// SPDX-License-Identifier: MIT

#include <algorithm>
#include <functional>
#include <vector>

#include <asn_1_int.h>

#include <doctest/doctest.h>
#include <outcome.hpp>

namespace glassy {
namespace test {

namespace outcome = OUTCOME_V2_NAMESPACE;

struct Asn1IntTestData {
  std::string subcase_name{};
  std::function<Result<Asn1Int>()> create{};
  Long value{0};
};

#define CALL(X) []() { return Asn1Int::X; }

SCENARIO("Asn1Int creation and conversions") {
  auto tests = std::vector<Asn1IntTestData>{
      {"default number", CALL(New()), 0},
      {"number 0", CALL(New(0)), 0},
      {"number 32", CALL(New(32)), 32},
  };

  std::for_each(std::begin(tests), std::end(tests),
                [](auto test) -> Result<void> {
                  SUBCASE(test.subcase_name.c_str()) {
                    OUTCOME_TRY(const auto number, test.create());
                    CHECK_EQ(number.ToLong().value(), test.value);
                  }

                  return outcome::success();
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
      return outcome::success();
    }()
                .value();
  }
}

SCENARIO("Asn1Int creation from pointer") {
  SUBCASE("creation from valid pointer should succeed") {
    CHECK(Asn1Int::Own(ASN1_INTEGER_new()).has_value());
  }

  SUBCASE("creation from nullptr should fail") {
    CHECK_FALSE(Asn1Int::Own(nullptr).has_value());
  }
}

SCENARIO("Asn1Int copy") {
  SUBCASE("create Asn1Int copy from original value") {
    []() -> Result<void> {
      OUTCOME_TRY(const auto original, Asn1Int::New(32));
      OUTCOME_TRY(const auto copy, Asn1Int::New(original));

      CHECK_NE(original.Ptr(), copy.Ptr());
      CHECK_EQ(original.ToLong().value(), copy.ToLong().value());
      CHECK_EQ(original, copy);
      CHECK_EQ(copy.ToLong().value(), 32);

      return outcome::success();
    }()
                .value();
  }
}

}  // namespace test
}  // namespace glassy
