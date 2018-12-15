// Copyright (C) 2018 Vincent Chambrin
// This file is part of the libscript library
// For conditions of distribution and use, see copyright notice in LICENSE

#include <experimental/filesystem>

int main()
{
  using namespace std;

  experimental::filesystem::path dir{ "." };

  if (experimental::filesystem::exists(dir))
  {
    int dircount = 0;
    int bobcount = 0;
    for (auto& entry : experimental::filesystem::directory_iterator(dir))
    {
      if (experimental::filesystem::is_directory(entry))
        dircount++;
      
      const experimental::filesystem::path path = entry;
      if (path.extension() == ".bob")
        bobcount++;
    }
  }

  return 0;
}
