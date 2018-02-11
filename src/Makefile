CXXFLAGS=-Wall -Wextra
LDFLAGS=-lwiringPi -lpng
OBJECTS=main.o image-processing.o

glow : $(OBJECTS)
	g++ -o $@ $(OBJECTS) $(LDFLAGS)

clean:
	rm -f $(OBJECTS) glow
