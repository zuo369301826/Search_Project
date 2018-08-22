PROTOC=../../third_part/bin/protoc

FLAG=-std=c++11 -I ../../third_part/include -L ../../third_part/lib\
		 -lsofa-pbrpc -lgflags -lglog -lprotobuf -lpthread\
		 -lz -lsnappy -lctemplate

client:client.cc server.pb.cc
	g++ $^ -o $@ $(FLAG)

server.pb.cc:server.proto
	$(PROTOC) server.proto --cpp_out=.
	
.PHONY:clear
	
clear:
	rm server.pb.* client
