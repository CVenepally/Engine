#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"

//------------------------------------------------------------------------------------------------------------------
bool FileReadToBuffer(std::vector<uint8_t>& outBuffer, const std::string& filename)
{

	FILE* file = nullptr;

	if(fopen_s(&file, filename.c_str(), "rb") != 0)
	{
	//	ERROR_RECOVERABLE(Stringf("Could not open file %s", filename.c_str()));
		return false;
	}

	if(fseek(file, 0, SEEK_END) != 0)
	{
		ERROR_RECOVERABLE(Stringf("SEEK TO THE END FAILED.\nFilename: %s", filename.c_str()));
		return false;
	}

	size_t fileSize = static_cast<size_t>(ftell(file));

	outBuffer.resize(fileSize);

	fseek(file, 0, SEEK_SET);

	if(fread(outBuffer.data(), sizeof(uint8_t), fileSize, file) != static_cast<size_t>(fileSize))
	{
		ERROR_RECOVERABLE(Stringf("Could not read file:.\nFilename: %s", filename.c_str()));
		return false;
	}

	fclose(file);

	return true;

}


//------------------------------------------------------------------------------------------------------------------
bool FileReadToString(std::string& outBuffer, const std::string& filename)
{	
	std::vector<uint8_t> vecBuffer;

	bool fileReadStatus = false;

	fileReadStatus = FileReadToBuffer(vecBuffer, filename);

	if(!fileReadStatus)
	{
		return false;
	}

	vecBuffer.push_back('\0');

	outBuffer = std::string(vecBuffer.begin(), vecBuffer.end());

	return true;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool DoesFileExist(std::string const& fileName)
{
	struct stat buffer;
	return (stat(fileName.c_str(), &buffer) == 0);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
int FileWriteFromBuffer(std::vector<uint8_t>& buffer, std::string const& fileName)
{
	FILE* file = nullptr;
	fopen_s(&file, fileName.c_str(), "wb");
	if(!file) 
	{
		return -1;
	}

	size_t bytesWritten = fwrite(buffer.data(), 1, buffer.size(), file);
	fclose(file);
	return static_cast<int>(bytesWritten);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string GetFileExtension(std::string filePath)
{
	int extensionIndex = static_cast<int>(filePath.find_last_of('.'));
	return filePath.substr(extensionIndex+1);
}
