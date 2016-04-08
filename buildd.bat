g++ -c -g -I ..\chipmunk\include -I ..\sfml\include *.cpp -Wall -std=c++11 2> buildlog.txt 
for %%f in (*.o) do move %%f obj\debug\%%f
g++ -L ..\chipmunk\lib -L ..\sfml\lib obj\debug\*.o -o bin\debug\debug.exe -lsfml-graphics -lsfml-window -lsfml-system -lchipmunk -mwindows 2> linklog.txt