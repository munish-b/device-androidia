#include "display_info.h"

Display::Display()
{
    sp<SurfaceComposerClient> client = new SurfaceComposerClient();

    DisplayInfo dinfo;
    sp<IBinder> display = SurfaceComposerClient::getBuiltInDisplay(
                            ISurfaceComposer::eDisplayIdMain);
    client->getDisplayInfo(display, &dinfo);

    height = dinfo.h;
    width = dinfo.w;
}

