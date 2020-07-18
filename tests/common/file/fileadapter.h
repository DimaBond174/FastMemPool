/*
 * This is the source code of SpecNet project
 *
 * Copyright (c) Dmitriy Bondarenko
 * feel free to contact me: specnet.messenger@gmail.com
 */

#ifndef FILEADAPTER_H
#define FILEADAPTER_H

#include <string>
#include <vector>
#include "network/udt/ipack.h"
#include <stdint.h>
//#if defined(Android)
//// https://github.com/android/ndk/issues/609
//// Only NDK R22 will support filesystem
//#include <experimental/filesystem>
//namespace  fs  =  std::experimental::filesystem ;
//#else
#include <filesystem>
namespace  fs  =  std::filesystem ;
//#endif

namespace spec {

struct file_info {
    fs::path path ;
    fs::file_time_type last_write_time ;
   // std::uintmax_t size ;
} typedef t_file_info;

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

std::vector<t_file_info>  file_list(const fs::path  &dir);
void  del_old_files(const std::string  &dir,  unsigned int  keepCount);
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

#endif // FILEADAPTER_H
