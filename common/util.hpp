#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <unordered_set>
#include <boost/algorithm/string.hpp>

namespace common{

  //字符串工具
  class StringUtil{
    public:
      //字符串分割
      static void Split(std::vector<std::string>* output, const std::string& input, const std::string& splic_char)
      {
        boost::split(*output, input, boost::is_any_of(splic_char), boost::token_compress_off);
      }

      static int32_t FindSentenceBeg(const std::string& content, int32_t first_pos)
      {
        for(int32_t i = first_pos; i > 0; --i )
        {
          if(content[i] == ';' || content[i] == ','||content[i] == '?' || content[i] == '!'
              || (content[i] == '.' && content[i+1] ==' '))
          {
            return i+1;
          }
        }
        return 0;
      }
  };

  //暂停词的处理
  class DictUtil
  {
    public:
    bool Load(const std::string& path)
    {
      std::ifstream file(path.c_str());
      if(!file.is_open())
      {
        return false;
      }
      std::string line;
      while(std::getline(file, line))
      {
        set_.insert(line);
      }
      return true;

    }
    bool Find(const std::string& key)
    {
      return set_.find(key) != set_.end();
    }

    private:
      std::unordered_set<std::string> set_;
  };


  class FileUtil{
        public:
    //从文件中读取数据
      static bool Read(const std::string& input_path, std::string* content)
      {
        std::ifstream file(input_path.c_str());
        if( !file.is_open()  ){
          return false;
        }

        file.seekg(0, file.end);
        int length = file.tellg();
        file.seekg(0, file.beg);
        content->resize(length);
        file.read(const_cast<char*>( content->data()), length );
        file.close();
        return true;
      }

      static bool Write(const std::string& output_path, const std::string& content)
      {
        std::ofstream file(output_path.c_str());
        if(! file.is_open()){
          return false;
        }
        file.write(content.data(), content.size());
        file.close();
        return true;
      }
  }; 
}
