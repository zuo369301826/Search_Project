#pragma once

#include "server.pb.h"
#include "../../index/cpp/index.h"

namespace doc_server {

typedef doc_server_proto::Request Request;
typedef doc_server_proto::Response Response;
typedef doc_index_proto::DocListNode DocListNode;
typedef doc_index::Index Index;

// 请求的上下文信息
struct Context {
  const Request* req;
  Response* resp;
  std::vector<std::string> words;//保存搜索请求的分词结果
  std::vector<const DocListNode*> all_query_chain;//保存触发结果，倒排拉链

  Context(const Request* request, Response* response)
    : req(request), resp(response) {  }
};

// 这个类是完成搜索的核心类
class DocSearcher {
public:
  // 搜索流程的入口函数
  bool Search(const Request& req, Response* resp);

private:
  // 对查询词进行分词
  bool CutQuery(Context* context);
  // 根据查询词结果进行触发
  bool Retrieve(Context* context);
  // 根据触发的结果进行排序
  bool Rank(Context* context);
  // 根据排序的结构拼装成响应
  bool PackageResponse(Context* context);
  // 打印请求日志
  bool Log(Context* context);
  // 排序需要的比较函数
  static bool CmpWeightPtr(const DocListNode* w1, const DocListNode* w2);
  // 生成描述信息
  std::string GenDesc(int first_pos, const std::string& content);
  // 替换 html 中的转义字符
  void ReplaceEscape(std::string* desc);
};
}  // end doc_server
