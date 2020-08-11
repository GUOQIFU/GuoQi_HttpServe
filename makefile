src = $(wildcard *.cpp)
obj = $(patsubst %.cpp,%.o,$(src))

all:guo_http

guo_http: condition.o threads_pool.o guo_http.o main.o 
	g++ -g condition.o threads_pool.o guo_http.o main.o -o guo_http -Wall -lpthread 
%.o:%.cpp
	g++ -g -c $< -Wall 
.PHONY:clean all
clean:
	-rm -rf guo_http $(obj) 





