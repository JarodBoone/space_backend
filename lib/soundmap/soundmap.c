// The main soundmap c++ file. witten in c++ for speed

// Necessary to extend the python programming interface and embed this binary 
// as a python module i.e. bind python API 
#define PY_SSIZE_T_CLEAN
#define RESULT_MEM 0x1 << 12
#define BUFFER_SIZE 0x1 << 12 // number of bytes in buffer 
#include "soundmap.h" 

// The mapping method of the soundmap module pretty much the entire module functionality
// Takes in an input sound and returns a string UUID associated with a user 
// static PyObject * soundmap_map(PyObject *self, PyObject *args) { 

//     const char *buffer = malloc(sizeof(char) * BUFFER_SIZE); 

//     // Parse python arguments 
//     // Note y* ==> bytes-like object best for passing binary data
//     if (!PyArg_ParseTuple(args,"y*",buffer)) 
//         return NULL; 

//     return PyLong_FromLong(0); 
// }

// static PyMethodDef SoundmapMethods[] = { 
//     {"map", NULL, METH_VARARGS, "Map input soundstream to username"}, 
//     {NULL,NULL,0,NULL} 
// };

// static struct PyModuleDef soundmapmodule = { 
//     PyModuleDef_HEAD_INIT, 
//     "soundmap", // name 
//     NULL, // currently no documentation
//     -1, // module state global 
//     SoundmapMethods, 
// };


// PyMODINIT_FUNC PyInit_soundmap(void) {
//     return PyModule_Create(&soundmapmodule); 
// }
 
int main(int argc, char **argv) { 
    wfl_t *results = (wfl_t *) malloc(sizeof(wfl_t));
    size_t *result_len;
    load_wav_dir("/home/jarod/space/db/users/test1/",results,result_len); 

    wfl_node_t *curr = results->head; 

    int o = feature_print(curr->node, NULL); 

    free(results); 
    return 0; 
}