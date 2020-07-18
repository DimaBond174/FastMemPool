/*
 * This is the source code of SpecNet project
 *
 * Copyright (c) Dmitriy Bondarenko
 * feel free to contact me: specnet.messenger@gmail.com
 */

#include "fileadapter.h"
#include <fstream>
#include <streambuf>
#include <sys/types.h>
#include "tools/specstatic.h"
#include <algorithm>
#include <cstring>

#if defined(Windows)
#include <direct.h>
#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif // !
#endif

bool  spec::createSymlink(const std::string  &target,  const std::string  &link)
{
  std::error_code  ec;
  fs::create_symlink(fs::path(target),  fs::path(link),  ec);
  return !ec;
}

std::vector<spec::t_file_info>  spec::file_list(const fs::path  &dir)
{
  std::vector<t_file_info>  result ;
  for  (const auto  &p  :  fs::recursive_directory_iterator(dir) )  {
    const auto  &path  =  p.path() ;
    if  (fs::is_regular_file(path))
      result.push_back( t_file_info  {
        path, fs::last_write_time(path)
        //,fs::file_size( path )
        } ) ;
    }
    return result ;
}

void  spec::del_old_files(const std::string  &dir,  unsigned int  keepCount)
{
  try  {
    auto&&  flist  =  file_list(dir) ;
    static const auto  cmp_times  =  [] (const t_file_info  &a,  const t_file_info  &b)
      {  return  (a.last_write_time  >  b.last_write_time);  } ;
    std::sort( std::begin( flist ),  std::end( flist ),  cmp_times ) ;
    for  (std::size_t  i  =  flist.size();  i  >  keepCount ;  --i)  {
      fs::remove( flist[i-1].path ) ;
    }
  }  catch  (const std::exception& ) {  }
}  //  del_old_files

void  spec::file_remove(const std::string &filePath)
{
  try {
    fs::remove(filePath);
  } catch (...) {

  }
  return;
}

void  spec::dir_remove(const std::string &dirPath)
{
  try {
    fs::remove_all(dirPath);
  } catch (...) {

  }
  return;
}

bool spec::file_exists(const std::string &filePath)
{
  bool  re = false;
  try {
    re  =  fs::exists(filePath);  //fs::exists(fs::path(path))  ;
  } catch(...) {}
  return re;
}

std::string spec::remove_dir_end_slash(const std::string &dirPath)
{
  auto pos = dirPath.size() - 1;
  for (; pos > 0; --pos) {
    char ch = dirPath[pos];
    if ('/' != ch && '\\' != ch) {  break; }
  }
  return dirPath.substr(0, pos + 1);
}

bool  spec::create_dir(const std::string &dirPath)
{  
  bool  re = fs::exists(dirPath);
  if (!re)
  {    
    try {  
      re  =  fs::create_directories(remove_dir_end_slash(dirPath));
    } catch(...) {

    }
  }
  return re;
}


bool spec::file_exists(const char * path)
{
  bool  re = false;
  try {
    re  =  fs::exists(path);  //fs::exists(fs::path(path))  ;
  } catch(...) {}
  return re;
}

std::string spec::getDir(const std::string  &filePath)
{
  size_t found  =  filePath.find_last_of("/\\");
  return(filePath.substr(0, found));
}

std::string spec::getFileName(const std::string  &filePath)
{
  size_t found  =  filePath.find_last_of("/\\");
  return(filePath.substr(found + 1, filePath.size() - found - 1));
}

std::string  spec::toFullPath(const std::string &path, const std::string &exe_path)
{
  std::string  re;
  if  (path.length() > 2)  {
    if('.' == path[0]  &&  '/' == path[1])  {
      re.append(exe_path).append(path.c_str() + 1);
    }  else  {
      re.append(path);
    }
  }
  return re;
}

bool spec::save_bin(const std::string &dir, const std::string &file_name, const char *data, uint32_t size)
{
  if (create_dir(dir))
  {
    std::string to_file(dir);
    to_file.push_back('/');
    to_file.append(file_name);
    save_bin(to_file, data, size);
    return true;
  }
  return false;
}

void spec::save_bin(const std::string &file_name, const char *data, uint32_t size)
{  
  std::ofstream fout(file_name, std::ios::out | std::ios::binary);
  fout.write(data, size);
  fout.close();
  return;
}

void spec::save_ipack(const std::string &file_path, IPack *pack)
{
  std::ofstream fout(file_path, std::ios::out | std::ios::binary);
  fout.write(reinterpret_cast<const char *>(pack->data), pack->size);
  fout.close();
  return;
}

uint64_t  spec::file_size(const char *file_name)
{
  if (std::ifstream is{ file_name, std::ios::binary | std::ios::ate })
  {
    auto re = is.tellg();
    if (re >= 0) return static_cast<uint64_t>(re);
  }
  return 0;
}

IPack * spec::read_ipack(const std::string &file_path)
{
  if (std::ifstream is { file_path, std::ios::binary | std::ios::ate })
  {
    auto size = is.tellg();
    if (size)
    {
      IPack *re = spec::create_IPack_malloc(static_cast<uint32_t>( size));
      if (re)
      {
        is.seekg(0);
        if (is.read(reinterpret_cast<char *>(re->data), size))
        {
          return re;
        }
      } else {
        spec::delete_IPack(re);
      }
    }
  }
  return nullptr;
}

std::vector<char> spec::read_bin(const char *file_name)
{
  if (std::ifstream is{ file_name, std::ios::binary | std::ios::ate })
  {
    auto size = is.tellg();
    std::vector<char> vec(static_cast<size_t>(size)); // construct string to stream size
    is.seekg(0);
    if (is.read(&vec[0], size))
    {
      return vec;
    }
  }
  return std::vector<char>();
}


std::string spec::read_text(const char *file_name)
{
  std::string  str;
  if (fs::exists(file_name)) {
    std::ifstream  t(file_name);
    t.seekg(0, std::ios::end);
    str.reserve(static_cast<size_t>(t.tellg()));
    t.seekg(0, std::ios::beg);
    str.assign((std::istreambuf_iterator<char>(t)),
      std::istreambuf_iterator<char>());
  }
  return str;
}

void spec::save_text(const std::string &dir, const char *file_name, const std::string &text)
{
  fs::create_directories(dir);
  std::string to_file(dir);
  to_file.append(file_name);//.append(".txt");
  std::ofstream outfile(to_file);
  outfile.write(text.c_str(), text.length());
  outfile.close();
} //save_text

bool spec::save_text(const char *file_name, const std::string &text)
{
  std::ofstream outfile(file_name);
  outfile.write(text.c_str(), text.length());
  outfile.flush();
  bool re = !outfile.bad();
  outfile.close();
  return re;
}

void spec::save_text_append(const std::string &dir, const char *file_name, const std::string &text)
{
  fs::create_directories(dir);
  std::string to_file(dir);
  to_file.append(file_name);//.append(".txt");
  //std::ofstream  outfile(to_file);
  std::ofstream outfile;
  outfile.open(to_file, std::ios_base::app);
  outfile.write(text.c_str(), text.length());
  outfile.close();
} //save_text

void spec::save_text_append(const char *file_name, const std::string &text)
{
  std::ofstream outfile;
  outfile.open(file_name, std::ios_base::app);
  outfile.write(text.c_str(), text.length());
  outfile.close();
}

bool  spec::isFileInFolder(const std::string &filePath,  const std::string &folderPath)
{
  auto && file_len = filePath.length();
  auto && folder_len = folderPath.length();
  if (folder_len > 0 && folder_len < file_len)
  {
//    for (auto i = folder_len - 1; i != 0; --i)
//    {
////      char fo_ch = folderPath[i];
////      char fi_ch = filePath[i];
////      if (fo_ch != fi_ch) return false;
//    } //for
    return  0 == strncmp(folderPath.c_str(), filePath.c_str(), folder_len);
  }
  return  false;
}
