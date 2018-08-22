#include <base/base.h>
#include <sofa/pbrpc/pbrpc.h>
#include "../../common/util.hpp"
#include "server.pb.h"
#include "searcher.h"

#include <iostream>

DEFINE_string(port, "10000", "服务器端口号");

DEFINE_string(index_path, "../index/index_file","索引文件的路径");

namespace doc_server {

typedef doc_server_proto::Request Request;
typedef doc_server_proto::Response Response;

//从doc_server_protr中继承过来
class DocServerAPIImpl : public doc_server_proto::DocServerAPI {
public:
  // 此函数是真正在服务器端完成计算的函数,需要对该rpc函数进行重写
  void Search(::google::protobuf::RpcController* controller,
         const Request* req,
         Response* resp,
         ::google::protobuf::Closure* done) 
  {
    //搜索核心算法
   doc_server::DocSearcher searcher; 
   searcher.Search(*req, resp);
    
    // 到这行代码表示服务器对这次请求的计算就完成了.
    // 由于 RPC 框架一般都是在服务器端异步完成计算,
    // 所以就需要由被调用的函数来通知调用者我计算完了.
    done->Run();
  }
};

}  // end doc_server


int main(int argc, char* argv[]) 
{
  base::InitApp(argc, argv); 
  using namespace sofa::pbrpc;
  
  std::cout<<"------------------   Index Load\n";
  
  // 0. 索引模块的加载和初始化  
  doc_index::Index* index = doc_index::Index::Instance();
  CHECK(index->Load(fLS::FLAGS_index_path));
  LOG(INFO) << "Index Load Done"; 

  std::cout<<"------------------   Index Load Done\n";

  //1.定义一个操作类
  RpcServerOptions option;
  option.work_thread_num = 4;

  //2. 用这option构建一个RpcServer对象
  RpcServer server(option);
  server.Start("0.0.0.0:" + fLS::FLAGS_port);

  //3. 定义一个函数对象，并注册到Rpcserver对象中
  doc_server::DocServerAPIImpl* service_impl 
    = new doc_server::DocServerAPIImpl();
  server.RegisterService(service_impl);
 
  std::cout<<"服务器已启动...\n";

  //4. 让RpcServer对象开始执行
  server.Run();

  LOG(INFO) << "server start";
  return 0;
}
