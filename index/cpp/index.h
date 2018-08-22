#include <string>
#include <vector>
#include <unordered_map>
#include "index.pb.h"
#include "../../common/util.hpp"
#include <cppjieba/Jieba.hpp>


struct WordCnt{
  int title_cnt;
  int content_cnt;
  int first_pos;

  WordCnt():
    title_cnt(0), content_cnt(0), first_pos(-1)
  {}
};

typedef std::unordered_map<std::string, WordCnt> WordCntMap;

namespace doc_index{
  typedef doc_index_proto::DocInfo DocInfo; // 文件信息 正排索引结点
  typedef doc_index_proto::DocListNode DocListNode; // 倒排拉链的结点
  typedef std::vector<DocInfo> ForwardIndex;// 正排索引
  typedef std::vector<DocListNode> InvertedList;// 倒排拉链
  typedef std::unordered_map<std::string, InvertedList> InvertedIndex; //倒排索引


  class Index{
    public:
      Index();

      static Index* Instance()
      {
        if(inst_ == NULL)
        {
          inst_ = new Index();
        }
        return inst_;
      }

      bool Build(const std::string& input_path);

      bool Save(const std::string& output_path);

      bool Load(const std::string& index_path);
  
      const DocInfo* GetDocInfo(uint64_t doc_id) const;

      const InvertedList* GetInvertedList(const std::string& key) const;

      void CutWordWithoutStopWord(const std::string& query,std::vector<std::string>* words);
    private:
      ForwardIndex forward_index_;
      InvertedIndex inverted_index_;
      cppjieba::Jieba jieba_;
      common::DictUtil stop_word_dict_;
      
      static Index* inst_;

      //私有成员函数
      const DocInfo* BuildForward(const std::string& line);
      void SplitTitle(const std::string& title, DocInfo* doc_info);
      void SplitContent(const std::string& content, DocInfo* doc_info);
      void BuildInverted(const DocInfo& doc_info);
      int CalcWeight(int title_cnt, int content_cnt);
      static bool CmpWeight(const DocListNode& w1, const DocListNode& w2);
      bool ConverToProto(std::string* proto_data);
      bool ConverFromProto(const std::string& proto_data);
      
      void SortInverted();
  };
}
