#include "VoxConverter.h"
#include <memory>
#include <filesystem>

namespace fs = std::experimental::filesystem;

int main()
{

	fs::path ConversionPath("./");
	fs::create_directory("ConvertedFiles");

	if (fs::exists(ConversionPath) && fs::is_directory(ConversionPath))
	{
		for (const auto& asset : fs::directory_iterator(ConversionPath))
		{
			if (asset.path().extension() == ".xraw")
			{
				std::unique_ptr<VoxelContainer> Container = ConvertXRAW(asset.path().string());
				WriteOVOX(std::string("./ConvertedFiles/") + asset.path().stem().string() + std::string(".ovox"), std::move(Container));
			}

		}
	}

	return 0;
}