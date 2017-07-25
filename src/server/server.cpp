//TODO - datestamp picture filenames
//TODO - datestamp multiline printing

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string>
using std::string;
#include <sstream>
using std::stringstream;

//3rd party includes
#include <pngwriter.h>

//my includes
#include "shared_data.h"

int multiline_plot(pngwriter& _image, char* _font_fn, char* _ch, int _fs, int _lh, int _x, int _y);

int main(int argc, char* argv[0])
{
    char* font_fn = "fonts/mononoki-Regular.ttf";
    key_t shm_key;
    int shm_id;
    struct Sh_mem_data* shm_ptr;

    shm_key = ftok(".", 'a');
    shm_id = shmget(shm_key, sizeof(struct Sh_mem_data), IPC_CREAT | 0666);
    if(shm_id < 0) {
        printf("*** ERROR: server failed to get shared memory ***\n");
        exit(1);
    }
    printf("Server received shared memory.\n");

    shm_ptr = (struct Sh_mem_data *) shmat(shm_id, NULL, 0);
    if((long int) shm_ptr == -1) {
        printf("*** ERROR: server failed to attach shared memory ***\n");
        exit(1);
    }
    printf("Server attached shared memory.\n");

    unsigned max_up = 180;
    for(unsigned i = 0; i < max_up; ++i) {
        printf("Listen uptime is %ds/%d.\n", i, max_up);
        printf("Reading shared memory.\n");
        printf("Shared memory ethernet link data:\n%s\n", shm_ptr->e_link_data);
        printf("Shared memory ethernet addr data:\n%s\n", shm_ptr->e_addr_data);
        printf("Shared memory wlan link data:\n%s\n", shm_ptr->w_link_data);
        printf("Shared memory wlan addr data:\n%s\n", shm_ptr->w_addr_data);
        printf("Will sleep for a bit.\n\n");

        string png_o_fld("png_out/");
        stringstream png_o_path;
        png_o_path << png_o_fld << "out" << i << ".png";
        int img_h = 100;
        int img_w = 1600;
        pngwriter image (img_w, img_h, 1.0, png_o_path.str().c_str());

        //Creating the background gradient
        double gradstart=0.0;
        double gradend=0.3;
        for(int h=1;h<=img_w;h++) {
            image.line(h,1,h,1000,1.0 -(gradend - gradstart)*h/1000, 1.0 -(gradend - gradstart)*h/1000, 1.0);
        }

        char* txt = &shm_ptr->e_link_data[0];
        image.plot_text_utf8(font_fn,14,25,80,0.0, txt,0.0,0.0,1.0);
        txt = &shm_ptr->e_addr_data[0];
        image.plot_text_utf8(font_fn,14,25,60,0.0, txt,0.0,0.0,1.0);
        txt = &shm_ptr->w_link_data[0];
        image.plot_text_utf8(font_fn,14,25,40,0.0, txt,0.0,0.0,1.0);
        txt = &shm_ptr->w_addr_data[0];
        image.plot_text_utf8(font_fn,14,25,20,0.0, txt,0.0,0.0,1.0);
        //multiline_plot(image, font_fn, txt, 14, 5, 25, 900);
        image.close();

        sleep(1);
    }

    shmdt((void *) shm_ptr);
    printf("Server has detached its shared memory...\n");
    shmctl(shm_id, IPC_RMID, NULL);
    printf("Server has removed its shared memory...\n");

    printf("Server is exiting...\n");
    exit(0);
}

int multiline_plot(pngwriter& _image, char* _font_fn, char* _ch, int _fs, int _lh, int _x, int _y)
{
    char* ch = _ch;
    char line[200];
    int line_no = 0;
    for(unsigned i = 0; *ch != '\0'; ++i) {
        line[i] = *ch;
        ++ch;
        if(line[i] = '\n') {
            line[i] = '\0';
            printf("line%d: %s", line_no, line);
            int y_dis = (_fs + _lh) * line_no;
            _image.plot_text_utf8(_font_fn,_fs,_x,_y - y_dis,0.0, &line[0],0.0,0.0,1.0);
            ++line_no;
        }
    }

    return 0;
}
