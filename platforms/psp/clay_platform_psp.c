// Sony PSP
#if !defined(_PSP)
#error "Must be using PSPDEV PSPSDK, Platform only valid for Sony PSP!"
#endif

#include <intraFont.h>
#include <math.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>
#include <string.h>

#include "clay_platform.h"

PSP_MODULE_INFO("Clay_Example", PSP_MODULE_USER, 1, 1);
PSP_HEAP_THRESHOLD_SIZE_KB(1024);
PSP_HEAP_SIZE_KB(-2048);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_MAIN_THREAD_STACK_SIZE_KB(1024);

#define BUF_WIDTH (512)
#define SCR_WIDTH (480)
#define SCR_HEIGHT (272)

extern void load(void);
extern void draw(void);
extern void leave(void);

static int running = 1;
static unsigned int __attribute__((aligned(16))) list[262144];

int exit_callback(int arg1, int arg2, void *common) {
  (void)arg1;
  (void)arg2;
  (void)common;
  running = 0;
  return 0;
}

int CallbackThread(SceSize args, void *argp) {
  (void)args;
  (void)argp;
  int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
  sceKernelRegisterExitCallback(cbid);
  sceKernelSleepThreadCB();
  return 0;
}

int SetupCallbacks(void) {
  int thid = sceKernelCreateThread("CallbackThread", CallbackThread, 0x11,
                                   0xFA0, PSP_THREAD_ATTR_USER, 0);
  if (thid >= 0) sceKernelStartThread(thid, 0, 0);
  return thid;
}

void Clay_Platform_Initialize(int width, int height, const char *title) {
  SetupCallbacks();

  // Init GU
  sceGuInit();
  sceGuStart(GU_DIRECT, list);

  sceGuDrawBuffer(GU_PSM_8888, (void *)0, BUF_WIDTH);
  sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, (void *)0x88000, BUF_WIDTH);
  sceGuDepthBuffer((void *)0x110000, BUF_WIDTH);

  sceGuOffset(2048 - (SCR_WIDTH / 2), 2048 - (SCR_HEIGHT / 2));
  sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);
  sceGuDepthRange(65535, 0);
  sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
  sceGuEnable(GU_SCISSOR_TEST);
  // sceGuDepthFunc(GU_GEQUAL);
  sceGuEnable(GU_DEPTH_TEST);
  sceGuFrontFace(GU_CW);
  // sceGuShadeModel(GU_SMOOTH);
  // sceGuEnable(GU_CULL_FACE);
  sceGuEnable(GU_CLIP_PLANES);
  sceGuEnable(GU_BLEND);
  sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
  sceGuFinish();
  sceGuSync(0, 0);

  sceDisplayWaitVblankStart();
  sceGuDisplay(GU_TRUE);
}

void Clay_Platform_Render_Start() {
  sceGuStart(GU_DIRECT, list);

  sceGumMatrixMode(GU_PROJECTION);
  sceGumLoadIdentity();
  sceGumPerspective(75.0f, 16.0f / 9.0f, 0.5f, 1000.0f);

  sceGumMatrixMode(GU_VIEW);
  sceGumLoadIdentity();

  sceGumMatrixMode(GU_MODEL);
  sceGumLoadIdentity();

  sceGuClearColor(0xFF7F7F7F); /* GRAY */
  sceGuClearDepth(0);
  sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
}

void Clay_Platform_Render_End() {
  // End drawing
  sceGuFinish();
  sceGuSync(0, 0);

  // Swap buffers (waiting for vsync)
  sceDisplayWaitVblankStart();
  sceGuSwapBuffers();
}

bool Clay_Platform_ShouldClose() { return running == 0; }
