GPP = g++
CURL_LIB = -lcurl
THRIFT_LIB = -lthrift
FLAGS = -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H -Wall
INCLUDE_DIR = -I/usr/local/include/thrift -I../gen-cpp
LD_DIR = -L/usr/local/lib

PROXY_CLIENT_SRC = proxy_client.cpp
PROXY_SERVER_SRC = proxy_server.cpp
PROXY_SERVER_LRU_SRC = proxy_server_lru.cpp
GEN_SRC = ../gen-cpp/proxy_constants.cpp ../gen-cpp/proxy_types.cpp ../gen-cpp/proxy.cpp

PROXY_CLIENT_TARGET = proxyclient
PROXY_SERVER_TARGET = proxyserver
PROXY_SERVER_LRU_TARGET = proxyserver_lru

all: $(PROXY_CLIENT_TARGET) $(PROXY_SERVER_TARGET) $(PROXY_SERVER_LRU_TARGET)

clean:
	rm -f $(PROXY_CLIENT_TARGET) $(PROXY_SERVER_TARGET) $(PROXY_SERVER_LRU_TARGET)

$(PROXY_CLIENT_TARGET): $(PROXY_CLIENT_SRC) $(GEN_SRC)
	$(GPP) $(FLAGS) $(INCLUDE_DIR) $(LD_DIR) $^ -o $@ $(THRIFT_LIB)

$(PROXY_SERVER_TARGET): $(PROXY_SERVER_SRC) $(GEN_SRC)
	$(GPP) $(FLAGS) $(INCLUDE_DIR) $(LD_DIR) $^ -o $@ $(THRIFT_LIB) $(CURL_LIB)

$(PROXY_SERVER_LRU_TARGET): $(PROXY_SERVER_LRU_SRC) $(GEN_SRC) ../gen-cpp/LRUCache.cpp
	$(GPP) $(FLAGS) $(INCLUDE_DIR) $(LD_DIR) $^ -o $@ $(THRIFT_LIB) $(CURL_LIB)	
