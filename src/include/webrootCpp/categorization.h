/*
 * categories.h
 *
 *  Created on: 12 Feb 2014
 *      Author: zytka
 */

#ifndef _SAFEKIDDO_CATEGORIES_H_
#define _SAFEKIDDO_CATEGORIES_H_

#include <vector>
#include <stdint.h>

#include "category.h"
#include "utils/typedInteger.h"
#include "bcsdklib.h"

namespace safekiddo
{
namespace webroot
{

TYPED_INTEGER(Confidence, int8_t);

class Categorization
{
public:
	explicit Categorization(TYPE_CC const webrootCC);

	static Categorization forCategory(Category category);

	Category getCategory() const;
	Confidence getConfidence() const;
	bool verify() const;

private:
	TYPE_CC webrootCC;
};

inline std::ostream & operator<<(std::ostream & out, Categorization const & self)
{
	return out << "(" << self.getCategory() << ", " << self.getConfidence() << ")";
}

typedef std::vector<Categorization> Categorizations;

// implementations

inline Categorization::Categorization(TYPE_CC const webrootCC):
	webrootCC(webrootCC)
{
}

inline bool Categorization::verify() const
{
	uint32_t const category = (this->webrootCC.CC & 0x7f00) >> 8;
	return category >= 1 && category <= 82;
}

inline Category Categorization::getCategory() const
{
	uint32_t const category = (this->webrootCC.CC & 0x7f00) >> 8;
	DASSERT(this->verify(), "unexpected", category, this->webrootCC.CC);
	return Category(category);
}

inline Confidence Categorization::getConfidence() const
{
	uint32_t const confidence = this->webrootCC.CC & 0x7f;
	DASSERT(confidence >= 0 && confidence <= 100, "unexpected", confidence);
	return confidence;
}

inline Categorization Categorization::forCategory(Category category)
{
	TYPE_CC const webrootCC = { static_cast<unsigned short>((category << 8) + 100) };
	return Categorization(webrootCC);
}

} // namespace webroot
} // namespace safekiddo

#endif /* _SAFEKIDDO_CATEGORIES_H_ */
