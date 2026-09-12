CXX = g++
CXXFLAGS = -Wall -O2 `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs` -lSDL2_ttf -lSDL2_image
SRC = $(wildcard src/*.cpp) $(wildcard stc/*.c)
OBJ = $(SRC:.cpp=.o)
TARGET = Q9Frame

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(TARGET)
