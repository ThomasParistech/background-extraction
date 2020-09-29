#include <iostream>
#include "Matrix.h"

using namespace std;

int main()
{
    Matrix game;
    game.load("sudoku.csv");
    game.solve();
    game.disp();
   // game.dispAllLinesPossible();
  //game.dispAllSquaresPossible();
   // game.dispAllNbrPossible();
    for(int val = 1; val<10;val++){
        //game.dispGridVal(val);
        game.doColoring(val-1);
    }
    game.disp();
    for(int val = 1; val<10;val++){
        game.dispGridVal(val);
    }
    game.save();
    cout << "Hello world!" << endl;
    while(true){}
    return 0;
}
