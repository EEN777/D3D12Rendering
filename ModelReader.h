#pragma once

#include "ContentTypeReader.h"
#include "Model.h"

namespace Library
{
	class ModelReader : public ContentTypeReader<Model>
	{
		RTTI_DECLARATIONS(ModelReader, AbstractContentTypeReader)

	public:
		ModelReader();
		ModelReader(const ModelReader&) = default;
		ModelReader& operator=(const ModelReader&) = default;
		ModelReader(ModelReader&&) = default;
		ModelReader& operator=(ModelReader&&) = default;
		~ModelReader() = default;

	protected:
		virtual std::shared_ptr<Model> _Read(const std::wstring& assetName) override;
	};
}