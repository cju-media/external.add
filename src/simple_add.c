/**
 * @file simple_add.c
 * @brief A simple Max external that adds two numbers.
 *
 * This external has two inlets and one outlet.
 * - Left inlet (inlet 0): Accepts int or float. Adds the value to the stored right operand and outputs the result.
 * - Right inlet (inlet 1): Accepts int or float. Stores the value as the right operand.
 */

#include "ext.h"
#include "ext_obex.h"

// Define the object structure
typedef struct _simple_add {
    t_object x_obj;     // The object header
    double r_val;       // Stored value from the right inlet
    void *x_out;        // Pointer to the outlet
} t_simple_add;

// Global class pointer
static t_class *simple_add_class = NULL;

// Function prototypes
void *simple_add_new(t_symbol *s, long argc, t_atom *argv);
void simple_add_float(t_simple_add *x, double f);
void simple_add_int(t_simple_add *x, long n);
void simple_add_ft1(t_simple_add *x, double f);

// Entry point
void C74_EXPORT ext_main(void *r) {
    t_class *c;

    // Create the class
    // "simple_add" - the name of the object
    // simple_add_new - the constructor
    // NULL - the destructor (default is sufficient here)
    // sizeof(t_simple_add) - size of the object struct
    // 0L - no menu function
    // A_GIMME - constructor accepts arbitrary arguments
    c = class_new("simple_add", (method)simple_add_new, (method)NULL, sizeof(t_simple_add), 0L, A_GIMME, 0);

    // Add methods
    class_addmethod(c, (method)simple_add_float, "float", A_FLOAT, 0);
    class_addmethod(c, (method)simple_add_int, "int", A_LONG, 0);

    // Method for float in the second inlet (index 1)
    // "ft1" is the standard message name for float in inlet 1
    class_addmethod(c, (method)simple_add_ft1, "ft1", A_FLOAT, 0);

    // Register the class
    class_register(CLASS_BOX, c);
    simple_add_class = c;
}

// Constructor
void *simple_add_new(t_symbol *s, long argc, t_atom *argv) {
    t_simple_add *x = (t_simple_add *)object_alloc(simple_add_class);

    if (x) {
        // Create inlets (right-to-left)
        // Inlet 1 (Right): Accepts floats (and ints via coercion if not handled otherwise)
        floatin(x, 1);

        // Inlet 0 (Left) is created automatically by object_alloc

        // Create outlets (right-to-left)
        // Outlet 0: Outputs float
        x->x_out = floatout(x);

        // Initialize state
        x->r_val = 0.0;

        // Process arguments if provided
        if (argc > 0 && atom_gettype(argv) == A_LONG) {
            x->r_val = (double)atom_getlong(argv);
        } else if (argc > 0 && atom_gettype(argv) == A_FLOAT) {
            x->r_val = atom_getfloat(argv);
        }
    }
    return x;
}

// Method for float input (Left Inlet)
void simple_add_float(t_simple_add *x, double f) {
    double result = f + x->r_val;
    outlet_float(x->x_out, result);
}

// Method for int input (Left Inlet)
void simple_add_int(t_simple_add *x, long n) {
    double result = (double)n + x->r_val;
    outlet_float(x->x_out, result);
}

// Method for float input (Right Inlet)
void simple_add_ft1(t_simple_add *x, double f) {
    x->r_val = f;
}
