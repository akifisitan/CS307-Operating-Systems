CXX = g++ 
CXXFLAGS = -lpthread

TARGET1 = court_test2
TARGET2 = court_test

SOURCE1 = court_test2.cpp
SOURCE2 = court_test.cpp

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(SOURCE1)
	$(CXX) $(SOURCE1) -o $(TARGET1) $(CXXFLAGS)

$(TARGET2): $(SOURCE2)
	$(CXX) $(SOURCE2) -o $(TARGET2) $(CXXFLAGS)

.PHONY: clean
clean:
	rm -f $(TARGET1) $(TARGET2)

sample10.1.1:
	g++ court_test.cpp -o court_test -lpthread

sample10.1.2:
	./court_test 4 5 3

sample10.1.3:
	./court_test 4 5 0

sample10.1.4:
	./court_test 4 4 1

sample10.1.5:
	./court_test 5 4 1

sample10.1.6:
	./court_test 12 4 1

sample10.1.7:
	./court_test 9 4 0

sample10.2.1:
	g++ court_test2.cpp -o court_test2 -lpthread

sample10.2.2:
	./court_test2 12 4 1

sample10.2.3:
	./court_test2 9 2 1
