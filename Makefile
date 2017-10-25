CC= g++
CPPFLAGS= -Wextra -Wall -g -std=c++11
HEADERS= CacheFS.h CacheFSContainer.h CachFSAlgos.h
TAR_FILES= CacheFS.cpp CacheFSContainer.cpp CacheFSContainer.h CachFSAlgos.cpp Answers.pdf CachFSAlgos.h

all: CacheFS.a

CacheFS.a: libCacheFS.a

libCacheFS.a: CacheFS.o CacheFSContainer.o CachFSAlgos.o
	ar rcs CacheFS.a $^

%.o: %.cpp $(HEADERS)
	$(CC) $(CPPFLAGS) -c $<

tar:
	tar -cvf ex4.tar $(TAR_FILES) README Makefile

clean:
	rm -rf *.o libCacheFS.a CacheFS.a

.PHONY: clean all tar
