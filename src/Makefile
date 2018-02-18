CXXFLAGS=-Wall -Wextra -std=c++11
LDFLAGS=-lwiringPi -lpng
OBJECTS=main.o image-processing.o glowxel-screen.o

glow : $(OBJECTS)
	g++ -o $@ $(OBJECTS) $(LDFLAGS)

clean:
	rm -f $(OBJECTS) glow
