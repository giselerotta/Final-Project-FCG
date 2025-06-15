cd 
cd repo/Final-Project-FCG/

sed -i 's/\.\.\/\.\.\///g' src/main.cpp

g++ src/*.cpp src/glad.c -Iinclude -o Laboratorio4 -lglfw -ldl -lGL

if [ $? -eq 0 ]; then
    echo "Compilation successful!"
    ./Laboratorio4
else
    echo "Compilation failed."
fi
