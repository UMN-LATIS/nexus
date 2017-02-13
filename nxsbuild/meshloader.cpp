#include "meshloader.h"

void MeshLoader::quantize(float &value) {
    if(!quantization) return;
    value = quantization*(int)(value/quantization);
}
