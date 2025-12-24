#ifndef CONFIG_H
#define CONFIG_H

typedef struct config_t {
    char keyAccel;
    char keyBrake;
    char keyLeft;
    char keyRight;
    char keyShiftUp;
    char keyShiftDn;
} config_t;

void loadOrInitConfig(config_t *config);
void writeConfig(config_t *config);

#endif

