#include <stdio.h>
#include <string.h>
#include "config.h"

void loadOrInitConfig(config_t *config) {
    FILE *fp = fopen("vheat.cfg", "rb");
    if (fp) {
        long len;
     	fseek(fp, 0, SEEK_END);
    	len = ftell(fp);
	    fseek(fp, 0, SEEK_SET);
        fread(config, sizeof(config_t), 1, fp);
        fclose(fp);
    }
    else {
        memset(config, 0, sizeof(config_t));
        config->keyAccel = 0x48;    // up
        config->keyBrake = 0x50;    // down
        config->keyLeft = 0x4b;     // left
        config->keyRight = 0x3d;    // right
        config->keyShiftUp = 0x48;  // a
        config->keyShiftDn = 0x50;  // z
    }
}

void writeConfig(config_t *config) {
    FILE *fp = fopen("vheat.cfg", "wb");
    fwrite(config, sizeof(config_t), 1, fp);
    fflush(fp);
    fclose(fp);
}

