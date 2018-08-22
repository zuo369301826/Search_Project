PROTOC=../../third_part/bin/protoc

FLAG=-std=c++11 -I ~/third_part/include -L ~/third_part/lib\
			 -L ../../index/cpp\
			 -lsofa-pbrpc -lprotobuf -lglog -lgflags -lpthread\
			 -lz -lsnappy

server:server.cc server.pb.cc searcher.cc ../../index/cpp/libindex.a
	g++ $^ -o $@ $(FLAG)

server.pb.cc:server.proto
	$(PROTOC) server.proto --cpp_out=.

.PHONY:clear

clear:
	rm server server.pb.*
