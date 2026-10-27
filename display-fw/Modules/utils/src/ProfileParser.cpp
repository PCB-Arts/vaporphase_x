#include "ProfileParser.hpp"

#include <algorithm>
#include <sstream>

extern "C"
{
#include "ff.h"
}

#include "log.h"

// Non-negative integer that fits into an int (std::stoi aborts otherwise, no
// exceptions)
bool IsNumber(std::string text)
{
  if (text.empty() || text.size() > 6)
  {
    return false;
  }
  for (char &c : text)
  {
    if (!std::isdigit(c))
    {
      return false;
    }
  }
  return true;
}

std::vector<std::string> FindCSVFiles()
{
  const std::string basepath = "profiles";
  DIR dir;
  FILINFO info;

  std::vector<std::string> files;
  if (f_findfirst(&dir, &info, basepath.c_str(), "*.CSV") == FR_OK)
  {
    do
    {
      std::string filename(info.fname);
      if (filename == "")
      {
        break;
      }
      files.push_back(basepath + "/" + filename);
    } while (f_findnext(&dir, &info) == FR_OK);
  }

  return files;
}

/* We rely on the file identifiers having the same size */
static_assert(sizeof(SOLDERPROFILE_FILEIDENTIFIER_VPX) ==
                  sizeof(SOLDERPROFILE_FILEIDENTIFIER_VPO),
              "Solder profile file identifiers must have the same length");

static bool IsSolderprofileIdentifier(const std::string &line)
{
  return line == SOLDERPROFILE_FILEIDENTIFIER_VPX ||
         line == SOLDERPROFILE_FILEIDENTIFIER_VPO;
}

bool IsSolderprofileCSV(std::string &filepath)
{
  FIL file;
  auto result = f_open(&file, filepath.c_str(), FA_READ);
  if (result != FR_OK)
  {
    return false;
  }

  std::string firstLine(sizeof(SOLDERPROFILE_FILEIDENTIFIER_VPX) - 1, '\0');
  unsigned int bytesRead;
  result = f_read(&file, &firstLine[0], firstLine.size(), &bytesRead);
  f_close(&file);

  if (result != FR_OK || bytesRead != firstLine.size())
  {
    return false;
  }

  return IsSolderprofileIdentifier(firstLine);
}

bool isProfileStart(std::string &line) { return line == "### Profile;"; }

int GetFilesize(std::string &filepath)
{
  FIL file;
  auto result = f_open(&file, filepath.c_str(), FA_READ);
  if (result != FR_OK)
  {
    return -1;
  }

  int filesize = 0;
  char tempBuffer[10];
  unsigned int bytesRead = 0;

  do
  {
    result = f_read(&file, tempBuffer, 10, &bytesRead);

    filesize += bytesRead;
  } while (result == FR_OK && bytesRead > 0);
  f_close(&file);
  return filesize;
}

SolderProfile loadSolderCsv(std::string &filepath)
{
  SolderProfile readError;
  readError.readError = true;

  int filesize = GetFilesize(filepath);
  if (filesize == -1)
  {
    return readError;
  }

  FIL file;
  if (f_open(&file, filepath.c_str(), FA_READ) != FR_OK)
  {
    return readError;
  }

  char *buffer = new char[filesize + 1];
  unsigned int bytesRead = 0;
  const FRESULT readResult = f_read(&file, buffer, filesize, &bytesRead);
  const FRESULT closeResult = f_close(&file);
  if (readResult != FR_OK || closeResult != FR_OK)
  {
    delete[] buffer;
    return readError;
  }
  buffer[bytesRead] = 0;

  std::string filecontent(buffer);
  delete[] buffer;
  return parseSolderCsv(filecontent);
}

SolderProfile parseSolderCsv(std::string &file)
{
  std::string normalized;
  normalized.reserve(file.size());

  for (size_t i = 0; i < file.size(); ++i)
  {
    if (file[i] == '\r')
    {
      if (i + 1 < file.size() && file[i + 1] == '\n')
        continue;
      normalized.push_back('\n');
    }
    else
    {
      normalized.push_back(file[i]);
    }
  }

  file.swap(normalized);
  std::istringstream stringStream(file);

  std::string line;
  std::getline(stringStream, line, '\n');

  SolderProfile profile;
  if (!IsSolderprofileIdentifier(line))
  {
    return profile;
  }

  std::getline(stringStream, line, '\n');

  // Parse Header
  while (!isProfileStart(line))
  {
    if (!stringStream)
    {
      vpx_log("Error, profile has no \"### Profile;\" line");
      return profile;
    }

    int splitterIndex = line.find(';');
    if (splitterIndex == -1 || splitterIndex == 0 ||
        splitterIndex == line.size() - 1)
    {
      vpx_log("Error, header line doesn't match required information %s",
              line.c_str());
    }
    else
    {
      auto key = line.substr(0, splitterIndex);
      auto value = line.substr(splitterIndex + 1, line.length());

      // Boiling temperature and ACT are sent to the core, a missing or
      // invalid value makes the profile invalid
      // -1 is out of both ranges
      const int number = IsNumber(value) ? std::stoi(value) : -1;
      if (key == "Boiling Temperature")
      {
        if (number <= 100 || number >= 250)
        {
          vpx_log("Error, boiling temperature \"%s\" is not an integer "
                  "between 100°C and 250°C",
                  value.c_str());
          return profile;
        }
        profile.boilingtemperature.push_back({number});
      }
      if (key == "ACT")
      {
        if (number <= 0 || number >= 150)
        {
          vpx_log("Error, Anti Condensation Temperature \"%s\" is not an "
                  "integer between 0°C and 150°C",
                  value.c_str());
          return profile;
        }
        profile.actemperature.push_back({number});
      }

      KeyValuePair attribute(key, value);
      profile.headerAttributes.push_back(attribute);
    }
    std::getline(stringStream, line, '\n');
  }

  if (profile.boilingtemperature.empty())
  {
    vpx_log("Error, profile has no \"Boiling Temperature\" line");
    return profile;
  }
  if (profile.actemperature.empty())
  {
    vpx_log("Error, profile has no \"ACT\" line");
    return profile;
  }

  // Skip table header
  std::getline(stringStream, line, '\n');

  // Parse Profile
  while (std::getline(stringStream, line, '\n'))
  {
    int splitterIndex = line.find(';');
    if (splitterIndex == -1 || splitterIndex == 0 ||
        splitterIndex == line.size() - 1)
    {
      vpx_log("Error, profile line doesn't match required information: %s",
              line.c_str());
    }
    else
    {
      auto time = line.substr(0, splitterIndex);
      auto temperature = line.substr(splitterIndex + 1, line.length());

      if (IsNumber(time) && IsNumber(temperature))
      {
        SolderTemperatureDatapoint datapoint;
        datapoint.temperature = std::stoi(temperature);
        datapoint.time = std::stoi(time);
        profile.timeTemperatureData.push_back(datapoint);
      }
      else
      {
        vpx_log("Error, profile line doesn't have the format time;temperature "
                "in integer numbers");
      }
    }
  }
  // The target curve needs a start and an end point, increasing times and has
  // to fit into SOLDERPROFILE_MAX_DURATION_S
  const auto &data = profile.timeTemperatureData;
  if (data.size() < 2)
  {
    vpx_log("Error, profile has less than 2 datapoints");
    return profile;
  }
  for (size_t i = 1; i < data.size(); i++)
  {
    if (data[i].time <= data[i - 1].time)
    {
      vpx_log("Error, profile times are not increasing at %d s", data[i].time);
      return profile;
    }
  }
  if (data.back().time >= SOLDERPROFILE_MAX_DURATION_S)
  {
    vpx_log("Error, profile is longer than %d s",
            SOLDERPROFILE_MAX_DURATION_S - 1);
    return profile;
  }

  vpx_log("Loading finished");
  profile.valid = true;
  return profile;
}
