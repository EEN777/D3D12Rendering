#include "pch.h"
#include "ContentTypeReader.h"

using namespace std;

namespace Library
{
	RTTI_DEFINITIONS(AbstractContentTypeReader)

	uint64_t AbstractContentTypeReader::TargetTypeId() const
	{
		return mTargetTypeId;
	}

	AbstractContentTypeReader::AbstractContentTypeReader(const uint64_t targetTypeId) :
		mTargetTypeId(targetTypeId)
	{
	}
}