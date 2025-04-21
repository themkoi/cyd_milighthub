#include "../src/mmWave.h"
#include "mmWaveWeb.h"


void generateMmwaveObject(JsonDocument &obj) {
  obj[FPSTR("Detected")] = isDetected;
}

void handleMmwave(RequestContext& request) {
  generateMmwaveObject(request.response.json);
}