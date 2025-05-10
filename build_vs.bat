md build_win
cd build_win
rem cmake ../ -DUSE_SELECT_IMPL=OFF -G"Visual Studio 17 2022" 
cmake ../ -DUSE_SELECT_IMPL=OFF -G"Visual Studio 17 2022" 
cd ..

md build_win_select
cd build_win_select
rem cmake ../ -DUSE_SELECT_IMPL=ON -G"Visual Studio 17 2022" 
cmake ../ -DUSE_SELECT_IMPL=ON -G"Visual Studio 17 2022" 
cd ..