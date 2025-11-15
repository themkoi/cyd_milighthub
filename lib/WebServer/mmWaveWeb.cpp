#include "../src/mmWave.h"
#include "mmWaveWeb.h"


void generateMmwaveObject(JsonDocument &obj) {
  obj[FPSTR("Detected")] = averageDetect;
}

void handleMmwave(RequestContext& request) {
  generateMmwaveObject(request.response.json);
}