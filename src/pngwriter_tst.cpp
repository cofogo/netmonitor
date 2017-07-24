#include <iostream>
using std::cout;
using std::endl;

#include "pngwriter.h"

int main()
{
    cout << "I'm alive!\n";

    char* font_fn = "fonts/mononoki-Regular.ttf";
    char* txt = "Testing the text.";
      
    //Creating the PNGwriter instance
    pngwriter image(1000,1000,1.0,"out.png");

    double gradstart=0.0;
    double gradend=0.3;
    //Creating the background gradient
    for(int h=1;h<=1000;h++) {
        image.line(h,1,h,1000,1.0 -(gradend - gradstart)*h/1000, 1.0 -(gradend - gradstart)*h/1000, 1.0);
    }

    image.plot_text_utf8(font_fn,14,25,870,0.0, txt,0.0,0.0,1.0);

    image.close();

    return 0;
}
