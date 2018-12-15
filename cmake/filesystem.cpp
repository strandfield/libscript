// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <filesystem>

int main()
{
  using namespace std;

  filesystem::path dir{ "." };

  if (filesystem::exists(dir))
  {
    int dircount = 0;
    int bobcount = 0;
    for (auto& entry : filesystem::directory_iterator(dir))
    {
      if (filesystem::is_directory(entry))
        dircount++;
      
      const filesystem::path path = entry;
      if (path.extension() == ".bob")
        bobcount++;
    }
  }

  return 0;
}
