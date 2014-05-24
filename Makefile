CC = g++
CPPFLAGS = -c -g -std=c++11 -Wall -pedantic-errors
LDFLAGS =

EXECUTABLE = expression_test

SOURCES = \
	expression_test.cpp \

DEPENDENCIES = $(SOURCES:.cpp=.d)
OBJECTS = $(SOURCES:.cpp=.o)

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.cpp
	$(CC) $(CPPFLAGS) $< -o $@

%.d: %.cpp
	@set -e; rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	rm -f $(EXECUTABLE) $(OBJECTS) $(DEPENDENCIES)

include $(DEPENDENCIES)

