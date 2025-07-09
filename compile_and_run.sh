g++ src/*.cpp src/glad.c -Iinclude -o Final-Project-FCG -lglfw -ldl -lGL

if [ $? -eq 0 ]; then
    echo "Compilation successful!"
    ./Final-Project-FCG
else
    echo "Compilation failed."
fi
