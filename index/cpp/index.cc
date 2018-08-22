#include <base/base.h>
#include <fstream>
#include "index.h"

DEFINE_string(dict_path, "../../third_part/data/jieba_dict/jieba.dict.utf8", "字典路径");                              
DEFINE_string(hmm_path, "../../third_part/data/jieba_dict/hmm_model.utf8", "hmm 字典路径");                            
DEFINE_string(user_dict_path, "../../third_part/data/jieba_dict/user.dict.utf8", "用户自定制词典路径");                
DEFINE_string(idf_path, "../../third_part/data/jieba_dict/idf.utf8", "idf 字典路径");                             
DEFINE_string(stop_word_path, "../../third_part/data/jieba_dict/stop_words.utf8", "暂停词字典路径");      

namespace doc_index{

  Index* Index::inst_ = NULL;

  Index::Index() : jieba_(fLS::FLAGS_dict_path,
                          fLS::FLAGS_hmm_path,
                          fLS::FLAGS_user_dict_path,
                          fLS::FLAGS_idf_path,
                          fLS::FLAGS_stop_word_path)
                          {
                            CHECK( stop_word_dict_.Load(fLS::FLAGS_stop_word_path));
                          }

      // -------------------------   Build   --------------------------
      void Index::SplitTitle(const std::string& title, DocInfo* doc_info)
      {
        //使用结巴分词
        std::vector<cppjieba::Word> words;
        //要调用 cppjieba 进行分词，需要先创建一个 jieba对象
        jieba_.CutForSearch(title, words);

        if(words.size() < 1)
        {
          LOG(FATAL) << "splitTitle failed title";
        }

        for(int i=0; i < words.size(); i++)
        {
          auto *token = doc_info->add_title_token();
          token->set_beg(words[i].offset);
          if(i + 1< words.size())
          {
            token->set_end(words[i+1].offset);
          }
          else 
          {
            token->set_end(title.size());
          }
        }
        return ;
      }
      
      void Index::SplitContent(const std::string& content, DocInfo* doc_info)
      {
        std::vector<cppjieba::Word> words;
        jieba_.CutForSearch(content, words);
        if(words.size() < 1){
          LOG(FATAL) << "SplitContent failed content";
        }

        for(int i=0; i < words.size(); ++i)
        {
          auto* token = doc_info->add_content_token();
          token->set_beg(words[i].offset);
          if(i + 1< words.size())
          {
            token->set_end(words[i+1].offset);
          }
          else 
          {
            token->set_end(content.size());
          } 
        }
        return ;
      }
 
      const DocInfo* Index::BuildForward(const std::string& line)
      {
        //1. 对字符串进行切割
        std::vector<std::string> tokens;
        common::StringUtil::Split(&tokens, line, "\3" );
        if(tokens.size() != 3){
          LOG(FATAL) << "line spilt not 3 token! tokens.size()="<<tokens.size();
        } 
        //2. 赋值
        DocInfo doc_info;
        doc_info.set_id(forward_index_.size());
        doc_info.set_title(tokens[1]);
        doc_info.set_content(tokens[2]);
        doc_info.set_jump_url(tokens[0]);
        doc_info.set_show_url(tokens[0]);
        
        //3. 对title和content进行分词处理
        SplitTitle(tokens[1],&doc_info);
        SplitContent(tokens[2],&doc_info);

        forward_index_.push_back(doc_info);
        return &forward_index_.back(); 
      }

      //构建倒排
      void Index::BuildInverted(const DocInfo& doc_info)
      {
        //统计每一个词出现的结果
        WordCntMap word_cnt_map;
        //1. 对一个文件的title分词结果进行处理
        for(int i=0; i < doc_info.title_token_size(); ++i)
        {
          const auto& token = doc_info.title_token(i);
          std::string word = doc_info.title().substr(token.beg(), token.end() - token.beg());//获取到单个分词
          boost::to_lower(word);//不区分大小写
          if(stop_word_dict_.Find(word)){//去掉暂停词
                  continue;
          }
          ++word_cnt_map[word].title_cnt;
        }

        //2. 统计content分词结果
        for(int i =0; i < doc_info.content_token_size(); ++i )
        {
          const auto& token = doc_info.content_token(i);
          std::string word = doc_info.content().substr(token.beg(), token.end() - token.beg());
          boost::to_lower(word);
          if(stop_word_dict_.Find(word)){
            continue;
          }
          ++word_cnt_map[word].content_cnt;
          if(word_cnt_map[word].content_cnt == 1)
            word_cnt_map[word].first_pos = token.beg();
        }

        //将个数的统计结果，更新到倒排索引中
        for( const auto& word_pair : word_cnt_map  )
        {
          DocListNode doc_list;
          doc_list.set_doc_id(doc_info.id());
          doc_list.set_weight(CalcWeight(word_pair.second.title_cnt, word_pair.second.content_cnt));
          doc_list.set_first_pos(word_pair.second.first_pos);
          inverted_index_[word_pair.first].push_back(doc_list);
        }
        return ;
      }
      int Index::CalcWeight(int title_cnt, int content_cnt)
      {
        return 10*title_cnt + content_cnt;

      }
      void Index::SortInverted()
      {
        for(auto & inverted_pair : inverted_index_)
        {
          InvertedList& inverted_list = inverted_pair.second;
          sort(inverted_list.begin(), inverted_list.end(), CmpWeight);
        }
        return ;
      }
      bool Index::CmpWeight(const DocListNode& N1, const DocListNode& N2)
      {
        return N1.weight() > N2.weight();
      }
      bool Index::Build(const std::string& input_path)
      {
        LOG(INFO) << "Index Build";
        // 1.按读取文件内容，针对每一行的数据进行处理
        //   构建正排索引，同时根据 DocInfo 构建倒排索引，最后将倒排拉链排序
        std::ifstream file(input_path);
        CHECK(file.is_open()) << "input_path:" << input_path;
        std::string line;
        while(std::getline(file, line))
        {
          //2. 将这一行数据做成一个DocInfo
          const DocInfo* doc_info =BuildForward(line);
          CHECK(doc_info != NULL);
          //3. 更新倒排拉链
          BuildInverted(*doc_info);
        }
        SortInverted();
        file.close();
        LOG(INFO) << "Index BUILD Done!!!";
        return true;
      }

//-------------------------  save ------------------------------
      bool Index::Save(const std::string& output_path)
      {
        LOG(INFO) << "Index Save";

        //1. 把内存中的索引结构序列化成字符串
        std::string proto_data;
        CHECK(ConverToProto(&proto_data));

        //2. 把序列化得到的字符串写到文件中
        CHECK(common::FileUtil::Write(output_path, proto_data));

        LOG(INFO) << "Index Save Done";
 
        return true;
      }

      bool Index::ConverToProto(std::string* proto_data)
      {
        //定义一个 proto 结构
        doc_index_proto::Index index;

        //序列化正排
        for(const auto& doc_info : forward_index_)
        {
          auto* proto_doc_info = index.add_forward_index();
          *proto_doc_info = doc_info;
        }
        
        //序列化倒排
        for(const auto& inverted_pair : inverted_index_)
        {
          auto* Kwd_info = index.add_inverted_index();
          Kwd_info->set_key(inverted_pair.first);
          for(const auto& Node : inverted_pair.second)
          {
            auto* proto_node = Kwd_info->add_doc_list();
            *proto_node = Node;
          }
        }
        index.SerializeToString(proto_data);
        return true;
      }


//-------------------------  Load ------------------------------
      bool Index::Load(const std::string& index_path)
      {
        LOG(INFO)<<"Index Load";

        std::string proto_data;
        CHECK(common::FileUtil::Read(index_path, &proto_data));


        LOG(INFO)<<"Read  Done";
        
        CHECK(ConverFromProto(proto_data));

        LOG(INFO)<<"Index Load Done";

        return true;
      }
    
      bool Index::ConverFromProto(const std::string& proto_data)
      {
        //反序列化得到 index 
        doc_index_proto::Index index;
        index.ParseFromString(proto_data);

        //生成正排索引
        for(int i = 0;i < index.forward_index_size(); ++i )
        {
          const auto& doc_info = index.forward_index(i);
          forward_index_.push_back(doc_info);
        }

        //生成倒排索引
        for(int i=0; i < index.inverted_index_size(); ++i )
        {
          const auto& Kwd_info = index.inverted_index(i);
          InvertedList& inverted_list =  inverted_index_[Kwd_info.key()];
          for(int j = 0; j < Kwd_info.doc_list_size(); ++j)
          {
            inverted_list.push_back(Kwd_info.doc_list(j));
          }
        }
        return true;
      }

        
  
//-------------------------  GetDocInfo ------------------------------
      const DocInfo* Index::GetDocInfo(uint64_t doc_id) const
      {
        if(doc_id >= forward_index_.size())
        {
          return NULL;
        }
        return &forward_index_[doc_id];
      }

//-------------------------  GetInvertedList  ------------------------------
      const InvertedList* Index::GetInvertedList(const std::string& key) const
      {
        auto it = inverted_index_.find(key);
        if( it == inverted_index_.end() )
          return NULL;

        return &(it->second);

      }

      void Index::CutWordWithoutStopWord(const std::string& query, std::vector<std::string>* words)
      {
        words->clear();
        std::vector<std::string> tmp;
        jieba_.CutForSearch(query, tmp);
        for(std::string& token : tmp){
          boost::to_lower(token);
          if(stop_word_dict_.Find(token)){
            continue;
          }
          words->push_back(token);
        }
      }
}
