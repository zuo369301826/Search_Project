#include "searcher.h"
#include <base/base.h>

DEFINE_int32(desc_max_size, 160, "描述的最大长度");

namespace doc_server{

  bool DocSearcher::Search(const Request& req, Response* resp){
    Context context(&req, resp);
   
    // 1. 对查询词进行分词
    CutQuery(&context);

    // 2. 根据分词结果进行触发
    Retrieve(&context);

    //3. 对结果进行排序
    DocSearcher::Rank(&context);

    //4. 构建相应
    PackageResponse(&context);

    //5 打印日志
    Log(&context);

    return true;
  }

  //--------------------------------------------------------------  分割
  bool DocSearcher::CutQuery(Context* context)
  {
    Index* index = Index::Instance();
    index->CutWordWithoutStopWord(context->req->query(), &context->words);
    LOG(INFO) << "CutQuery Done!sid=" << context->req->sid();
    return true;
  }

  
  //--------------------------------------------------------------  触发
  bool DocSearcher::Retrieve(Context* context)
  {
    Index* index = Index::Instance();
    //根据所有的分词结果，到索引中找到所有的倒排拉链
    for(const auto& word : context->words  )
    {
      const doc_index::InvertedList* inverted_list = index->GetInvertedList(word);
      if(inverted_list == NULL)
      {
        continue;
      }
      for(size_t i = 0; i< inverted_list->size(); ++i){
          const auto& Node = (*inverted_list)[i];
          context->all_query_chain.push_back(&Node);
      }
      //到这里所有倒排拉链信息就都放到数组里了
      return true;
    }
  }
//--------------------------------------------------------  排序
  
  bool DocSearcher::Rank(Context* context)
  {
    //对所有结果进行排序，已经不区分不同关键字了
    std::sort(context->all_query_chain.begin(),
        context->all_query_chain.end(),
        CmpWeightPtr);
      return true;
  }

  bool DocSearcher::CmpWeightPtr(const DocListNode* d1,const DocListNode* d2)
  {
    return d1->weight() > d2->weight();
  }

// ---------------------  构建响应 --------------------
  bool DocSearcher::PackageResponse(Context* context)
  {
    Index* index = Index::Instance();
    const Request* req = context->req;
    Response* resp = context->resp;
    resp->set_sid(req->sid());
    for(const auto* Node : context->all_query_chain )
    {
      //查正排，根据doc_id，获取到文档的属性
      const auto* doc_info = index->GetDocInfo(Node->doc_id());

      //doc_info 数目和返回结果中的item是意义对应的
      auto* item = resp->add_item();
      item->set_title(doc_info->title());

      item->set_desc(GenDesc(Node->first_pos(),doc_info->content()));//筛取描述
      item->set_jump_url(doc_info->jump_url());
      item->set_show_url(doc_info->show_url());
    }
    return true;
  }

  std::string DocSearcher::GenDesc(int first_pos, const std::string& content)
  {
    //1. 确定开始的位置
    int64_t desc_beg = 0;
    if( first_pos != -1 )
    {
      //先前查找
      desc_beg = common::StringUtil::FindSentenceBeg(
          content, first_pos);
    }

    // 2. 从句子开始的位置，往后查找若干个字节
    std::string desc;
    if( desc_beg + fLI::FLAGS_desc_max_size >= (int32_t)content.size())
    {
      desc = content.substr(desc_beg);
    }else 
    {
      desc = content.substr(desc_beg, fLI::FLAGS_desc_max_size);
      desc[desc.size() -1 ] = '.';
      desc[desc.size() -2 ] = '.';
      desc[desc.size() -3 ] = '.';
    }

    //替换特殊字符
    ReplaceEscape(&desc);
    return desc;
  }

  void DocSearcher::ReplaceEscape(std::string* desc)
  {
    boost::algorithm::replace_all(*desc, "&", "&amp;");
    boost::algorithm::replace_all(*desc, "\"", "&quot;");
    boost::algorithm::replace_all(*desc, "<", "&lt;");
    boost::algorithm::replace_all(*desc, ">", "&gt;");
  }


  //-------------------------------------- 打印日志
  bool DocSearcher::Log(Context* context)
  {
    LOG(INFO) << "[Request]" << context->req->Utf8DebugString(); 
    LOG(INFO) << "[Response]" << context->resp->Utf8DebugString(); 
    return true;
  }
  
}
