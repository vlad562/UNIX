#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <openssl/sha.h>
#include <sstream>
#include <string>
#include <unordered_map>

using namespace std;
namespace fs = std::filesystem;

string fileHash(const fs::path &filePath)
{
  ifstream input(filePath, ios::binary);

  if (!input)
    throw runtime_error("Не могу найти файл: " + filePath.string());

  SHA_CTX sha;
  SHA1_Init(&sha);

  char data[4096];

  while (input.read(data, sizeof(data)) || input.gcount() > 0)
  {
    SHA1_Update(&sha, data, input.gcount());
  }

  unsigned char hashValue[20];
  SHA1_Final(hashValue, &sha);

  return string(reinterpret_cast<char *>(hashValue), 20);
}

int main()
{
  string folder;

  cout << "Введите директорию: ";
  getline(cin, folder);

  if (!fs::is_directory(folder))
  {
    cout << "Директория не найдена\n";
    return 1;
  }

  unordered_map<string, fs::path> hashMap;

  for (const auto &entry : fs::recursive_directory_iterator(folder))
  {
    if (!entry.is_regular_file())
      continue;

    fs::path path = entry.path();

    try
    {
      string hash = fileHash(path);

      if (hashMap.count(hash) == 0)
      {
        hashMap.insert({hash, path});
        cout << "Сохранен: " << path << endl;
      }
      else
      {
        fs::path oldFile = hashMap[hash];

        fs::remove(path);
        fs::create_hard_link(oldFile, path);

        cout << "Ссылка создана: " << path << " - " << oldFile << endl;
      }
    }
    catch (const exception &err)
    {
      cout << "Ошибка: " << err.what() << endl;
    }
  }

  return 0;
}
