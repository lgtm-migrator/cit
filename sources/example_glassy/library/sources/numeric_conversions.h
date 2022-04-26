#ifndef NUMERIC_COVERSIONS_H
#define NUMERIC_COVERSIONS_H

#include <asn_1_int.h>
#include <big_num.h>

namespace glassy {
namespace convert {

namespace outcome = OUTCOME_V2_NAMESPACE;

template <typename T>
using Result = outcome::result<T>;

static Result<BigNum> FromAsn1Int(const Asn1Int& value) noexcept;
static Result<Asn1Int> FromBigNum(const BigNum& value) noexcept;

inline Result<BigNum> FromAsn1Int(const Asn1Int& value) noexcept {
  auto result = BigNum::Own(ASN1_INTEGER_to_BN(value.Ptr(), nullptr));
  if (result.has_error()) {
    return BigNumErrc::kConversionFailure;
  }
  return result;
}

inline Result<Asn1Int> FromBigNum(const BigNum& value) noexcept {
  auto result = Asn1Int::Own(BN_to_ASN1_INTEGER(value.Ptr(), nullptr));
  if (result.has_error()) {
    return Asn1IntErrc::kConversionFailure;
  }
  return result;
}

}  // namespace convert
}  // namespace glassy

#endif  // NUMERIC_COVERSIONS_H
