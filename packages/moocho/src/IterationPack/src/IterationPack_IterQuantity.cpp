// /////////////////////////////////////////////////////
// IterQuantity.cpp

#include "GeneralIterationPack/src/IterQuantity.hpp"
#include "ThrowException.hpp"

namespace GeneralIterationPack {

void IterQuantity::assert_has_storage_k(int offset) const {
	if(!has_storage_k(offset))
		THROW_EXCEPTION(
			true, NoStorageAvailable
			,"IterQuantity::assert_has_storage_k(offset) : There is not storage available for "
				<< name() << " for the k"	<< std::showpos << offset << " iteration" );
}

void IterQuantity::assert_updated_k(int offset) const {
	if(!updated_k(offset))
		THROW_EXCEPTION(
			true, QuanityNotSet
			,"IterQuantity::assert_updated_k(offset) : The interation quantity " << name() 
			<< " has not been been updated for the k"	<< std::showpos << offset
			<< " iteration yet." );
}

} // end namespace GeneralIterationPack
