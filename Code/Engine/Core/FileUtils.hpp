#pragma once

#include <vector>
#include <string>

//------------------------------------------------------------------------------------------------------------------

bool	FileReadToBuffer(std::vector<uint8_t>& outBuffer, const std::string& filename);
bool	FileReadToString(std::string& outBuffer, const std::string& filename);

bool	DoesFileExist(std::string const& fileName);
int		FileWriteFromBuffer(std::vector<uint8_t>& buffer, std::string const& fileName);

std::string GetFileExtension(std::string filePath);