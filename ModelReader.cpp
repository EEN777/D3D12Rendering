#include "pch.h"
#include "ModelReader.h"
#include "Utility.h"

using namespace std;

namespace Library
{
	RTTI_DEFINITIONS(ModelReader)

	ModelReader::ModelReader() :
		ContentTypeReader(Model::TypeIdClass())
	{
	}

	shared_ptr<Model> ModelReader::_Read(const wstring& assetName)
	{
		return make_shared<Model>(Utility::ToString(assetName));
	}
}