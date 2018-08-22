#include <base/base.h>
#include <sofa/pbrpc/pbrpc.h>
#include <ctemplate/template.h>
#include "server.pb.h"

#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

DEFINE_string(server_addr, "127.0.0.1:10000", "请求的搜索服务器的地址");

DEFINE_string(template_path,"./wwwroot/template/search_page.html", "模板文件的路径");

namespace doc_client
{

  typedef doc_server_proto::Request Request;
  typedef doc_server_proto::Response Response;

  int GetQueryString(char* output)
  {
    //1. 获得方法
    char* method = getenv("METHOD");
    if(method == NULL)
    {
      fprintf(stderr, "Get METHOD failed\n");
      return -1;
    }

    //如果是 get 方法直接在获取环境变量
    if(strcasecmp(method, "GET") == 0 )
    {
      char* query_string = getenv("QUERY_STRING");
      if(query_string == NULL)
      {
        fprintf(stderr, "QUERY_STRING failed\n");
        return -1;
      }
      strcpy(output, query_string);
    }
    else //如果是POST方法
    {
      char* content_length_str = getenv("CONTENT_LENGTH");
      if(content_length_str == NULL)
      {
        fprintf(stderr, "CONTENT_LENGTH failed\n");
        return -1;
      }
      int content_length = atoi(content_length_str);
      for(int i=0; i < content_length; ++i)
      {
        read(0, &output[i], 1);
      }
      output[content_length] = '\0';
    }
    return 0;
  }

  void PackageRequset(Request* req)
  {
    req->set_sid(0);

    //获取query_string   buf包含 query=***  字符串
    char buf[1024] = {0};
    GetQueryString(buf);

    //获得参数
    char query[1024] = {0};
    sscanf(buf, "query=%s", query);
    req->set_query(query);
  }

  void Search(const Request& req, Response* resp)
  {
    using namespace sofa::pbrpc;

    //1. 先定义一个 RPC client 对象
    RpcClient client;
    
    //2. 再将该对象作为参数，绑定上端口号，定义一个 RPCChannel 连接对象
    RpcChannel channel(&client, fLS::FLAGS_server_addr);

    //3. 定义一个服务函数对象，参数是连接对象
    doc_server_proto::DocServerAPI_Stub stub(&channel);

    //4. 定义一个 ctrl 对象，用于网络控制
    RpcController ctrl;
    ctrl.SetTimeout(3000);

    //5. 等待调用，相当于调用了远端服务器的函数
    stub.Search(&ctrl, &req, resp, NULL);
  
    //6. 判断是否调用成功
    if( ctrl.Failed() )
    {
      fprintf(stderr, "PRC Search failed\n");
    }else {
      fprintf(stderr, "PRC Search OK\n");
    }
  }

  void ParseResponse(const Response& resp)
  {
    //输出一个 HTML 结果
    //使用 ctemplate 完成页面的构造
    ctemplate::TemplateDictionary dict("SearchPage");
    for(int i =0; i < resp.item_size(); ++i)
    {
      ctemplate::TemplateDictionary* table_dict
        = dict.AddSectionDictionary("item");
      table_dict->SetValue("title",resp.item(i).title());
      table_dict->SetValue("desc",resp.item(i).desc());
      table_dict->SetValue("jump_url",resp.item(i).jump_url());

      table_dict->SetValue("show_url",resp.item(i).show_url());

    }

    //加载模板文件
    ctemplate::Template* tpl = ctemplate::Template::GetTemplate(fLS::FLAGS_template_path,
        ctemplate::DO_NOT_STRIP);

    std::string html;
    tpl->Expand(&html, &dict);
    std::cerr<<html;
    std::cout<<html;
    return;
  }

  //此函数为客户端请求服务区的入口函数
  void CallServer()
  {
    Request req;
    Response resp;

    //1. 构造响应
    PackageRequset(&req);

    //2. 从服务器获取到相应并解析
    Search(req, &resp);

    //3. 解析响应吧结果输出出来
    ParseResponse(resp);

    return;
  }

}


int main(int argc, char* argv[])
{
  base::InitApp(argc, argv);
  doc_client::CallServer();
  return 0;
}
