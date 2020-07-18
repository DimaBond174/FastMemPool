#ifndef CFILEADAPTER_H
#define CFILEADAPTER_H

#include <string>
#include <vector>
#include "network/udt/ipack.h"

namespace spec {

bool save_bin(const std::string &dir,  const std::string &file_name,
                     const char *data, uint32_t  size);

void save_bin(const std::string &file_name,
                     const char *data, uint32_t  size);

void save_ipack(const std::string &file_path, IPack *pack);

std::vector<char> read_bin(const char *file_name);
IPack * read_ipack(const std::string &file_path);

std::string read_text(const char *file_name);

void save_text(const std::string &dir, const char *file_name,
                      const std::string &text);

bool save_text(const char *file_name,
                      const std::string &text);

void save_text_append(const std::string &dir, const char *file_name,
                      const std::string &text);

void save_text_append(const char *file_name,
                      const std::string &text);

std::string  toFullPath(const std::string &path, const std::string &exe_path) ;
std::string  getDir(const std::string  &filePath) ;
std::string  getFileName(const std::string  &filePath) ;

bool  file_exists(const char * path)  ;
bool  file_exists(const std::string &filePath)  ;
bool  create_dir(const std::string &dirPath)  ;
bool  createSymlink(const std::string  &target,  const std::string  &link);
bool  isFileInFolder(const std::string &filePath,  const std::string &folderPath);
uint64_t  file_size(const char *file_name);
void  file_remove(const std::string &filePath);
void  dir_remove(const std::string &dirPath);
std::string remove_dir_end_slash(const std::string &dirPath);
} // spec

#endif // CFILEADAPTER_H
