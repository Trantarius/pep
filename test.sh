g++ -w -O3 -std=c++17 -I. $(find src -name "*.cpp") -o exe
echo
echo compiled
echo
./exe

