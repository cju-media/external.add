/**
 * @file jit.stream.c
 * @brief A Max/Jitter external to stream video data to an RTMP server using FFmpeg.
 */

#include "jit.common.h"
#include "jit.matrix.h"

#ifdef WIN_VERSION
#define popen _popen
#define pclose _pclose
#endif

typedef struct _jit_stream {
    t_object    ob;
    void        *obex;

    t_symbol    *url;       // RTMP URL
    double      framerate;  // Target framerate
    long        bitrate;    // Bitrate in kbits (optional)

    FILE        *pipe;      // Pipe to FFmpeg process
    long        width;
    long        height;
    long        planecount;
    t_symbol    *type;
} t_jit_stream;

void *jit_stream_new(t_symbol *s, long argc, t_atom *argv);
void jit_stream_free(t_jit_stream *x);
void jit_stream_assist(t_jit_stream *x, void *b, long m, long a, char *s);
t_max_err jit_stream_jit_matrix(t_jit_stream *x, t_symbol *s, long argc, t_atom *argv);
void jit_stream_close(t_jit_stream *x);
t_max_err jit_stream_notify(t_jit_stream *x, t_symbol *s, t_symbol *msg, void *sender, void *data);

static t_class *jit_stream_class = NULL;

void C74_EXPORT ext_main(void *r) {
    t_class *c;

    jit_init(); // Initialize Jitter

    c = class_new("jit.stream", (method)jit_stream_new, (method)jit_stream_free,
                  sizeof(t_jit_stream), 0L, A_GIMME, 0);

    // Standard Max methods
    class_addmethod(c, (method)jit_stream_assist, "assist", A_CANT, 0);
    class_addmethod(c, (method)jit_stream_notify, "notify", A_CANT, 0);

    // Jitter Matrix input
    class_addmethod(c, (method)jit_stream_jit_matrix, "jit_matrix", A_GIMME, 0);

    // Close message to force stop stream
    class_addmethod(c, (method)jit_stream_close, "close", 0);

    // Attributes
    CLASS_ATTR_SYM(c, "url", 0, t_jit_stream, url);
    CLASS_ATTR_DOUBLE(c, "framerate", 0, t_jit_stream, framerate);
    CLASS_ATTR_LONG(c, "bitrate", 0, t_jit_stream, bitrate);

    // Default values
    CLASS_ATTR_DEFAULT(c, "framerate", 0, "30.0");
    CLASS_ATTR_DEFAULT(c, "bitrate", 0, "2500");

    // Standard Obex support
    class_addmethod(c, (method)object_obex_dumpout, "dumpout", A_CANT, 0);
    class_addmethod(c, (method)object_obex_quickref, "quickref", A_CANT, 0);

    class_register(CLASS_BOX, c);
    jit_stream_class = c;
}

void *jit_stream_new(t_symbol *s, long argc, t_atom *argv) {
    t_jit_stream *x = (t_jit_stream *)object_alloc(jit_stream_class);

    if (x) {
        x->obex = object_obex_alloc(x, NULL, 0); // Create obex (for attributes)

        // Initialize defaults
        x->url = _jit_sym_nothing;
        x->framerate = 30.0;
        x->bitrate = 2500;
        x->pipe = NULL;
        x->width = 0;
        x->height = 0;
        x->planecount = 0;
        x->type = _jit_sym_char;

        // Process attributes
        attr_args_process(x, argc, argv);

        // Dumpout outlet
        object_obex_dumpout_alloc(x, NULL);
    }
    return x;
}

void jit_stream_free(t_jit_stream *x) {
    jit_stream_close(x);
    object_obex_free(x);
}

void jit_stream_assist(t_jit_stream *x, void *b, long m, long a, char *s) {
    if (m == ASSIST_INLET) {
        sprintf(s, "jit_matrix input");
    } else {
        sprintf(s, "dumpout");
    }
}

void jit_stream_close(t_jit_stream *x) {
    if (x->pipe) {
        pclose(x->pipe);
        x->pipe = NULL;
        object_post((t_object *)x, "Stream closed.");
    }
    x->width = 0;
    x->height = 0;
}

t_max_err jit_stream_jit_matrix(t_jit_stream *x, t_symbol *s, long argc, t_atom *argv) {
    t_symbol *matrix_name;
    void *matrix;
    long dim[2];
    long planecount;
    t_symbol *type;
    t_jit_matrix_info info;
    char *bp;
    char cmd[2048];

    if (argc < 1 || atom_gettype(argv) != A_SYM)
        return MAX_ERR_GENERIC;

    matrix_name = atom_getsym(argv);
    matrix = jit_object_findregistered(matrix_name);

    if (!matrix || !jit_object_method(matrix, _jit_sym_class_jit_matrix))
        return MAX_ERR_GENERIC;

    // Lock matrix
    long savelock = (long)jit_object_method(matrix, _jit_sym_lock, 1);

    jit_object_method(matrix, _jit_sym_getinfo, &info);

    // Validate format (only supporting char for now)
    if (info.type != _jit_sym_char) {
        object_error((t_object *)x, "Only char matrices are supported.");
        jit_object_method(matrix, _jit_sym_lock, savelock);
        return MAX_ERR_GENERIC;
    }

    // Check if configuration changed or pipe is closed
    if (!x->pipe || x->width != info.dim[0] || x->height != info.dim[1] || x->planecount != info.planecount) {
        jit_stream_close(x);

        if (x->url == _jit_sym_nothing || x->url->s_name[0] == '\0') {
             // No URL set, just return
             jit_object_method(matrix, _jit_sym_lock, savelock);
             return MAX_ERR_NONE;
        }

        // Determine pixel format based on planecount
        const char *pix_fmt = "argb"; // default for 4 planes
        if (info.planecount == 1) pix_fmt = "gray";
        else if (info.planecount == 3) pix_fmt = "rgb24"; // Jitter 3 plane is usually RGB
        else if (info.planecount == 4) pix_fmt = "argb"; // Jitter 4 plane is usually ARGB

        x->width = info.dim[0];
        x->height = info.dim[1];
        x->planecount = info.planecount;

        // Construct FFmpeg command
        // Note: Using ultrafast preset and flv format for RTMP
        snprintf(cmd, sizeof(cmd),
            "ffmpeg -y -f rawvideo -vcodec rawvideo -pix_fmt %s -s %ldx%ld -r %.2f -i - -c:v libx264 -b:v %ldk -maxrate %ldk -bufsize %ldk -preset ultrafast -tune zerolatency -f flv \"%s\"",
            pix_fmt, x->width, x->height, x->framerate, x->bitrate, x->bitrate, x->bitrate * 2, x->url->s_name);

        object_post((t_object *)x, "Opening FFmpeg pipe: %s", cmd);

        #ifdef WIN_VERSION
        x->pipe = popen(cmd, "wb");
        #else
        x->pipe = popen(cmd, "w");
        #endif

        if (!x->pipe) {
            object_error((t_object *)x, "Failed to open FFmpeg pipe.");
            jit_object_method(matrix, _jit_sym_lock, savelock);
            return MAX_ERR_GENERIC;
        }
    }

    // Get data pointer
    jit_object_method(matrix, _jit_sym_getdata, &bp);

    if (bp && x->pipe) {
        // Write frame to pipe
        // Calculate size: width * height * planecount * sizeof(char)
        size_t framesize = x->width * x->height * x->planecount;
        size_t written = fwrite(bp, 1, framesize, x->pipe);
        if (written != framesize) {
             // Broken pipe?
             object_error((t_object *)x, "Error writing to pipe.");
             jit_stream_close(x);
        }
    }

    jit_object_method(matrix, _jit_sym_lock, savelock);
    return MAX_ERR_NONE;
}

t_max_err jit_stream_notify(t_jit_stream *x, t_symbol *s, t_symbol *msg, void *sender, void *data) {
    return object_obex_notify(x, s, msg, sender, data); // Handle attribute notifications
}
