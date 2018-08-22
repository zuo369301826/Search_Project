PROTOC=../../third_part/bin/protoc
FLAG=-std=c++11 -I ../../third_part/include -L ../../third_part/lib \
		 -lpthread -lprotobuf -lgflags -lglog

.PHONY: all

all:libindex.a build

libindex.a:index.cc index.pb.cc
	g++ -c index.cc -o index.o $(FLAG)
	g++ -c index.pb.cc -o index.pb.o $(FLAG)
	ar -rc libindex.a index.o index.pb.o

build:build.cc
	g++ $^ -o $@ -L . -lindex $(FLAG)

index.pb.cc:index.proto
	$(PROTOC) index.proto --cpp_out=.


.PHONY:clear
clear:
	rm *.pb.cc *.pb.h build *.o *.a

