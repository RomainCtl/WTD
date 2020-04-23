# Makefile pour les programmes OpenGL du livre D-BookeR
# note: sudo apt-get install libglfw3-dev libglew-dev libsdl2-dev libsdl2-image-dev libopenal-dev libalut-dev

# nom du programme à construire
EXEC = main

# Server conf
SERVER_CONFIG_FILE = server_config.json
SERVER_ADDR = 127.0.0.1
SERVER_PORT = 8080

# liste des modules utilisateur : tous les .cpp (privés de cette extension) du dossier courant
MODULES = $(basename $(wildcard [A-Z]*.cpp))

# liste des modules de libs : tous les .cpp (privés de cette extension) du dossier libs
MODULES_LIBS = $(basename $(wildcard libs/*.cpp libs/*/*.cpp))

# liste des dossiers à inclure : tous ceux de libs
MODULES_INCS = $(sort $(dir $(wildcard libs/*/*.h)))

# options de compilation et librairies
CXXFLAGS = -std=c++11 -I. -Ilibs $(addprefix -I,$(MODULES_INCS)) -I/usr/include/SDL2 -g # -O3
LIBS = -lGLEW -lGL -lGLU -lglfw -lSDL2 -lSDL2_image -lopenal -lalut -lpthread


#### Ne pas modifier au delà (sauf si vous savez ce que vous faites)

# édition des liens entre tous les fichiers objets
$(EXEC): .o/main.o $(patsubst %,.o/%.o,$(notdir $(MODULES))) $(addsuffix .o,$(MODULES_LIBS))
	$(CXX) -o $@ $^ $(LIBS)

# exécution du programme
run:	$(EXEC)
	./$(EXEC) $(SERVER_ADDR) $(SERVER_PORT)

# exécution du programme
run-p:	$(EXEC)
	./$(EXEC) $(SERVER_ADDR) $(SERVER_PORT) -p

# compilation d'un module
.o/%.o: %.cpp $(addsuffix .h,$(MODULES)) | .o
	$(CXX) $(CXXFLAGS) -c $< -o .o/$(notdir $@)

# compilation des librairies
libs/%.o: libs/%.cpp libs/%.h

# dossier .o/
.o:
	mkdir -p .o

# exécution avec vérification de la mémoire
valgrind:	$(EXEC)
	valgrind --track-origins=yes --leak-check=full --num-callers=30 ./$(EXEC) | tee valgrind.log

# vérification avec glslangValidator
glslang:	$(EXEC)
	for f in *.vert ; do glslangValidator $${f} $${f%.vert}.frag ; done

# icone
icon:	run
	-convert -quality 95 image.ppm ../$(shell basename $(dir $(CURDIR))).jpg

# nettoyage complet : l'exécutable est supprimé aussi
cleanall: clean
	rm -f main image.ppm

# nettoyage du projet et des librairies
cleanalllibs:	cleanall cleanlibs

# nettoyage des fichiers objets et logs du projet
clean:
	rm -rf .o *.log *.vert *.frag *~

# suppression des fichiers objets des librairies
cleanlibs:
	rm -fr $(addsuffix .o,$(MODULES_LIBS))

build-serv: ## To build server
	g++ -Wall -Wextra -Wconversion -ansi -Wpedantic -std=gnu++11 server.cpp -o server -ljsoncpp -lpthread

run-serv: build-serv ## To run server
	./server $(SERVER_PORT) $(SERVER_CONFIG_FILE)
