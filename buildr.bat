g++ -c -I ..\chipmunk\include -I ..\sfml\include *.cpp -Wall -std=c++11 2> buildlog.txt 
for %%f in (*.o) do move %%f obj\release\%%f
g++ -L ..\chipmunk\lib -L ..\sfml\lib obj\release\*.o -o bin\release\release.exe -lsfml-graphics -lsfml-window -lsfml-system -lchipmunk -mwindows 2> linklog.txt