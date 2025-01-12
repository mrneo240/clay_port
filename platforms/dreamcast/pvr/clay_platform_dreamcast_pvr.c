// Sega Dreamcast
#if !defined(_arch_dreamcast)
#error "Must be using KallistiOS, Platform only valid for Sega Dreamcast!"

#endif

#include "dc_renderer_includes.h"

pvr_init_params_t pvr_params = (pvr_init_params_t){
    .opb_sizes = {PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0,
                  PVR_BINSIZE_0},
    1024 * 1024};

#include "clay_platform.h"

static void processInput(void) {
  static inputs _input;
  unsigned int buttons;

  maple_device_t *cont;
  cont_state_t *state;

  cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
  if (!cont) return;
  state = (cont_state_t *)maple_dev_status(cont);

  buttons = state->buttons;

  /*  Reset Everything */
  memset(&_input, 0, sizeof(inputs));

  /* DPAD */
  _input.dpad = (state->buttons >> 4) & ~240;  // mrneo240 ;)

  /* BUTTONS */
  _input.btn_a = (uint8_t)!!(buttons & CONT_A);
  _input.btn_b = (uint8_t)!!(buttons & CONT_B);
  _input.btn_x = (uint8_t)!!(buttons & CONT_X);
  _input.btn_y = (uint8_t)!!(buttons & CONT_Y);
  _input.btn_start = (uint8_t)!!(buttons & CONT_START);

  /* ANALOG */
  _input.axes_1 = ((uint8_t)(state->joyx) + 128);
  _input.axes_2 = ((uint8_t)(state->joyy) + 128);

  /* TRIGGERS */
  _input.trg_left = (uint8_t)state->ltrig & 255;
  _input.trg_right = (uint8_t)state->rtrig & 255;

  INPT_ReceiveFromHost(_input);
}

void Clay_Platform_Initialize(int width, int height, const char *title) {
  cont_btn_callback(0, CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y,
                    (cont_btn_callback_t)arch_exit);

  setbuf(stdout, NULL);
  setvbuf(stdout, NULL, _IONBF, 0);

  pvr_init_defaults();
}

void Clay_Platform_Render_Start() {
  processInput();

  vid_border_color(0, 0, 0);
  pvr_wait_ready();
  vid_border_color(255, 0, 0);
  pvr_scene_begin();
}

void Clay_Platform_Render_End() {
  pvr_scene_finish();
  vid_border_color(0, 255, 0);
}

bool Clay_Platform_ShouldClose() { return false; }

void Clay_Platform_Shutdown() {
  // not sure
}
